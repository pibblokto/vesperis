# Lessons by area

What earlier milestones learned the hard way, moved out of the session memory in E-01 (2026-10-23). The short rules live in the
CLAUDE.md files next to the code; this is the detail behind them, one lesson a bullet. E-04 folds it into the rules by area.
Read only the section you need: `grep -n '^## ' docs/reference/lessons.md`.

<!-- contents -->
Contents:
- Working on the dev Mac
- Proving and timing
- The harness
- Pixels and the look
- Space
- The surface
- Generation: the worlds
- Generation: the peoples
- The records' text
- Game, screens and sound
<!-- /contents -->

## Working on the dev Mac

- Scratch builds use `/usr/bin/clang++`, as the Makefile does: Homebrew's clang 22 comes first on PATH.
- The Makefile links Homebrew's static `libraylib.a`: the shared one hides GLFW's joystick functions.
- macOS `sed` has no `\b`: script edits in python (and escape `\n` inside C++ literals). `grep -E` lookaheads fail under ugrep.
  `make | grep` dies of SIGPIPE: build into a log, then grep it. `xargs -I{}` fails on long commands: use `xargs -n 1 -P 8`.
- zsh: `"a b c"` stays one word (`${=w}` splits it); an unquoted heredoc runs backticks; `rm` of an unmatched glob aborts the
  command (add `(N)`); `time` prints "total".
- make's timestamps have one-second resolution: after copying or restoring sources, touch them a second later or delete their
  `.o`. Never `shutil.copy2` sources (it keeps the old times).
- After any scripted edit, read what it touched: a regex once rewrote lambdas into recursion, and a doc rewrap merged entries
  (count the `## ` and `* **` lines before and after).
- Redirected output is block-buffered, so a stalled log is not a hang (`ps -o etime,time`; `sample <pid>` shows where it is).
  Find a crash with `lldb --batch -o run -o "bt all" -- <command>`.
- Run long builds and harness runs in the background with the output in a file; wait with an until-grep loop, not chains of sleeps.
- Keep tools, baselines, proof kits and toolchains under `build/`: the session scratchpad and /private/tmp are wiped.
- Copy only the PNGs a run writes (`find -newer <marker>`) and delete them afterwards: a full disk stops the Bash tool.
- The Read tool shows a 320x200 frame too small to judge small elements: crop and enlarge it first (sips, or python).
- The window build autosaves into its working directory: run it from a scratch directory, under `caffeinate -d -i` (with `-i`
  alone GLFW finds no monitor). The user's own game may be running (`/tmp/vesperis`): alternate A/B runs.
- A GNU compound literal in a range-for dangles: use a named array.
- The Makefile's `CXX ?= /usr/bin/clang++` never applied: make's own default `CXX = c++` is already set, so `?=` keeps it (the
  same Apple clang). E-02 sets the path when CXX is make's default, and keeps `-Isrc -MMD -MP` in CPPFLAGS whatever flags are
  passed.

## Proving and timing

- A scratch program linking `build/{core,galaxy,space,surface,game}/*.o` (written as globs, not a zsh variable) reproduces or
  times in seconds what the unit takes minutes for. With the harness's objects too (all but `harness/main.o`; it defines
  `g_testScale`) and a `TestGame` subclass, it is a differential: linked against the old objects before an edit and the new
  after, U-04's scan held every state's controller pages, names, buttons and drawn menus under 79 knobs to the old in seconds.
- Prove a refactor against the old binary built from a scratch tree (`git worktree add --detach`, `git archive`, or the old
  sources with the newest harness), keeping the old arithmetic. A generation change is proven by stream and per-body fingerprint
  diffs against the old sources: the draws identical and the peoples' worlds unchanged.
- Keep an old formula exact when a new term is zero: add the term after the old sum, never inside it.
- A scratch oracle (the old code verbatim against the new, on random inputs) proves a rewritten integer pass. Prove a cone or edge
  test with a sweep where the effect shows (near a glow's edge an added term rounds to 0).
- `regress` hashes few frames and no HUD: prove the HUD, the cabin, forms and screens with `flow`, `screens` and harness frames;
  anything animated with `stability <scene>`.
- Timings: `unit` 3-6 min, `flow` 35-45 s, `bench check` about 40 s, `regress` seconds.
- A run far slower with identical output is the environment (sleep, load, battery): run it again alone, beside back-to-back
  control runs of the previous binary.
- Before timing, check the power, `top -l 2 -s 2` and `sysctl vm.swapusage`. On battery a batch drifts 20% or more: compare
  only within one batch.
- Judge a hot-path change by alternating A/B pairs (ABBA) against the previous binary in `build/perf-baselines` (`benchcmp.py`).
- The launch path, the binary's name, the working directory's length and the code's layout move rows: run A/B from paths of equal
  length, swap the names once, and judge final binaries. The sector map 4x row moved 6.2-8.6 ms by its launch name alone, so swap
  the names before calling a row under ~10% real.
- Time the parts before trusting a plan's hotspot. A short row after a faster one can read slower: time the view alone over
  hundreds of frames before calling a regression.
- Compiler flags are not the lever, code is (-O3 and LTO stayed within the noise on the Mac).
- Profile with `sample` (1 ms) on `bench forest`, or the bench under a pty. The game's mush kernel at 4x is 3x3, the forest
  bench's 5x5.
- raylib's `GetTime` stops in sleep and `steady_clock` does not: set aside runs that slept. Tag every run uniquely: a repeated
  tag overwrites its CSVs and log. `rot.sh` resolves its binaries before its `cd`.
- The buggy 2x bench row is noisy (drainage work). Isolate a pool change with the old pool behind the new interface.
- `tools/prove` (E-02) is the proof: a snapshot of the binary, the objects and every item's outputs, compared item by item. Run
  it alone: D-05's `trait_ice_halos` sweep frame differed from today's in 18 px by one step, and three runs alone all matched
  today's (D-05's was drawn beside other jobs).
- The flow is deterministic from an empty `shots/tests`: it makes every save it loads, and two runs draw the same 179 files once
  the saves' `saved <unix time>` line and the timings are masked. The unit passes from an empty `shots/tests` too (403 checks):
  the records check's settings file falls back to the defaults. Its six scanner and map frames (R-408, B-412, T-03) and its
  autosave are not repeatable: it runs frames until a thread has found the cells or the scan, so the trees and grass sway, and
  the autosave's clock ends, where the threads' pace leaves them. Two runs back to back can agree and a third differ:
  `tools/prove.items` names such files (`noise=`) with the reason, besides what a `twice` snapshot sees move.
- Relink an old tree's saved objects with the newest harness to give a new fingerprint column a base
  (`PROVE_BIN=<binary> tools/prove snapshot`): take the link line from `make -n -B vesperis_test`, since the order of the
  objects decides the order of start-up.
- Prove a comment-only or whitespace-only step by the objects (`tools/prove objects`): the engine's and the game's compile
  byte for byte the same; the harness's too but for three of its forty (E-03: clang varies `voicePrint`, `unitGiants` and
  `unitSky` between compiles), so the harness is proven by behaviour, and a move of its code by `tools/samecode.py`.
- Long runs at night met macOS's media analysis and Spotlight indexing the snapshots' thousands of pictures (`mediaanalysisd`,
  `mds_stores`): `tools/prove` items that take 9-34 s took about 15 minutes each, the process held off at nice 5 (E-02). Keep
  `build/` and `shots/` out of Spotlight (System Settings, Spotlight's privacy list), and see the load first (`top -o cpu`).

## The harness

- Test worlds: Aieliaalas II (151 0 25, body 1, felisian), Leileashphail III (153 0 70, body 2, desert), Eleinewai I (152 92 0,
  two peoples), Seehio II (151 70 1), Yiothal III (152 50 2). A generation rule ripples into the pinned worlds (`AGE_WORLDS`, the
  two peoples, the wet dam, the radar salts): pin them again by scans.
- Keep one `Game` alive in the harness (a second corrupts the first); reach its state through TestGame's hooks
  (`src/harness/test_game.h`), never from the game's own files; join worker threads at exit (`atexit`).
- The flow's frames are 1x, the unit's and the modes' 4x; a mode's checks may also run in the unit. A `Game`'s `applySettings`
  sets the global render scale and mush (its settings say 4x): every group of the unit after a scripted game draws at 4x, so
  `UNIT_GROUPS` states each group's starting scale and mush, and a group alone draws as in the full run (E-03).
- Check the check before the generator: `fmt()` cuts at 512 characters (split the details), prints truncate, and substring counts
  hit field words ("cooldown").
- Unit checks span several worlds, decays and every instance (one world's doorways hid two defects), and require a rare feature
  across worlds. Checks over kinds stop at their family's last id, so new kinds don't break them.
- Checks find sites on the plan (`planPlacePoint`), never at fixed coordinates. A check that assumes one people's ways pins the
  genome (`genome.set`, then `cultureWays`). A test world's default region can be sea: find coasts by bisecting the shoreline.
- Harness walks go to the nearest instance (works cluster round towns) and stand where the wanted object is the nearest;
  multi-world checks take one pass per world. Collider walks check every frame, allow sliding round thin forms, and reason out
  each sample's expectation.
- A new negative scene code goes in every finder list: the `wantMat <= -30` exclusions, the -47 group, the trait filter, the
  stand points, the scene print. Stands are searched, printed and pinned: the effect visible (clear sky, the sun not ahead), dry,
  clear of colliders, in sight, a summit for far views, and on a small moon within its 1-2 km horizon.
- A scene finder measures what the drawn ground draws (drainage on) and puts the explorer on the spot; `forTypeBodies` and the
  scenes try real bodies before staged ones. Civilisation scenes stand on many worlds: diff the old and new scene logs.
- Look at cabin frames, not only bare renders, and render degenerate configurations (a one-deck giant, a brown dwarf) too.
- Test an overlay against the function at the texel's centre, not bilinearly at the site. Show a meteor shower as a max-composite
  over seconds; a sky direction tested over minutes takes the local frame anew. Telescope plates persist across stows and steps:
  a fresh-plate check uses a power never used.
- Input hooks: `testTypeText` ends with Enter; a rename starts from the old name (clear it); `testStatusNext` waits for the
  status; `testParkAt` opens the landing map and waits for it unless `waitMap=false`. Shots wait for asynchronous maps
  (`testWaitMapOf`), or they show the binary's speed.
- Menus are walked by row index: append rows, or fix every KEY_DOWN count in the flow and the unit. Flow steps that add game time
  shift the later frames and the autosave: place them before the probe block, and abort flights at once.
- A game frame reproduces only when every thread wait happens at one instant: all maps waited (`testWaitMapOf`), `frame(in, 0)`,
  held still. Warm a bench row at dt 0 after a scale change (its first frames carry allocations).
- `load landing` and `load map` never open an unmapped world; `load capital 6 4 0` does (trees); the default `load capital` none.
- The bench's 13 stable PNGs compare; capital, decoder, landmap, landzoom, photo, radar, sectormap and telescope vary.
- The sweeps differ by scan order: compare against the skip build. A frame that flips between runs is first a body drawn before its
  map's thread finished (the chemotroph's sweep frame, E-02: the sweeps wait for every map now). Run them alone (a busy machine
  moved 18 px of a trait sheet).
- Reproduce the user's state on copies of their `guide.txt` and `vesperis_save_auto.txt` in a scratch directory.
- A frame with bodies in view waits for every map first (`testWaitMapOf`, or `SpaceRenderer::mapIfReady` on a view's own
  renderer): the maps are made on their thread at the binary's own pace (F-07), so without the wait a frame moves between runs
  by a faint disc or a dim background (E-02 found the data sheets, the surface sweep, the unit's lent piece and the rogues'
  radar lock).
- A race can resolve the same way in every run of one binary and the other way in a binary whose code moved: E-02's radar lock
  frame lost the parked world's map in three relinks of the base engine and won it in the final one, though the code that draws
  it was the same instructions. A twice-run cannot see it; a move or split (which shifts every later address) shows it. Prove
  such a change innocent by the objects' instructions (objdump, labels and addresses stripped) and by the frame with the wait.
- The harness is `src/harness/` (E-03), linked only into vesperis_test: a one-file edit rebuilds in 1-3 s (the one 2 MB file
  took 35 s).
- Splitting one file: a static function the split shares loses `static` and gains a declaration, its default arguments moved
  into it; then whole chunks move (`tools/mover.py`, which rebuilds the old file from the new ones to prove it). Compare each
  function's IR at -O0 and without exceptions (`tools/samecode.py`): with exceptions, clang's `invoke`s and inferred `nounwind`
  follow the order it emits functions in, which a split changes. A chunk keeps the comments above it, so a comment the file
  had already cut off from its function travels with the wrong one (E-03: formsChecks' and agesChecks' sat above roomOpenness).
- A unit check's fixed start point can lie in an ocean. The flow's `testCabinGoto` takes a pitch (a desk under the window band
  needs -0.65 from 1.5 m).
- Pictures that print a time differ between runs by design: the landforms sheets' "COST N US", the town map's "IN N MS".
- `charts rogues` named a drifter's chart's wanderers from the drifter's own system, but their indices are its former system's
  (`peoplesWorldOf`): it read past `sys.bodies` and printed freed memory, a different line each run (E-02 fixed the printout).

## Pixels and the look

- Overlays on the space view skip pixels with `Framebuffer::invz` set, or they draw across the cabin's walls.
- `drawLineRGB` neither clips nor draws single pixels (it draws S-wide blocks): clip first, and write faint map lines into
  `canvas.px`. `drawText`'s scale multiplies the canvas's. 2-D helpers take `RGBCanvas&`, not `Framebuffer`. `rgb()` packs red
  low (0xFFBBGGRR).
- The tone tile eats albedo steps under ~40% (steps of 20-28% vanished): judge ground marks by pixel columns, not by eye.
- The sky bank runs from zenith to horizon: measure the sky column by pixel before tuning; a dark element on a coloured sky needs
  its own bank.
- The mush averages shades across bank edges: two banks meet cleanly only where their ramps agree in light and colour over the
  shades both reach.
- A scene's darkness scales the ramps' colours, not the shades; a lamp or a flash raises the frame's light itself
  (L = max(day, lamp, flash)). Colour-mixing layers (fog, haze, flash) composite in RGB with RGB ramp tables: a bank holds one
  colour per shade.
- A sub-pixel silhouette is one framebuffer pixel at its own shade: paling it toward the fog makes ghosts. A point on a globe must
  outlive the mush: FB_SCALE+2 wide.
- The banks are allocated per view (`docs/reference/02-rendering.md`'s table for each scene), each one thing within it: the
  globe's 31 is body A's, the surface's 25-27 the peoples' (skipped when test-staged).
- Per-frame visuals get their own cheap noise (a gradient table at 4 ns against `gnoise2`'s 40 ns). Anchor sampling grids in
  world angle and distance (no shimmer); blend noise values, never coordinates, between frames; noise round a centre uses
  (cos, sin) times the arc's radius.
- Never call `getenv` on a per-pixel path (10 ms a frame). A per-pixel test shows at 4x, not 2x: bench pairs at 4x catch it.
  Judge a libm replacement in the frame, on the renderer's own input order, never by a micro-benchmark.
- The pool: progressive work goes in slices inside the frame (worker timing depends on the machine). Decide pool changes at 4x;
  a pool on every core starves the game's own threads; one `notify_all` beats several `notify_one`; a row's cost counts pixels.

## Space

- Suns draw after the galactic band (an earlier glow punched holes in the Milky Way); the band's ramp has no black foot.
- A black hole's lens shows what lies behind it: a pixel kept there becomes an Einstein ring.
- A body lit only by the sky is drawn in the sky's exposure, never brighter than the sky behind it.
- A per-pixel lookup on a frame-filling globe costs ~5 ms at 2x: gate it by a coarse mask, and keep its reads free of modulo.
  Read a body's kind tags once per globe, not per pixel.
- Judge a flight's stability over the whole flight, the band and the near content separately, then look at the worst frame.
- A parked ship sits inside every moon's orbit (no transits).
- Probe decks: hole walls take the upper deck's colour; tie layers to decks, not pressures; print a field's profile along the
  heading; clamp to the limb's dip.

## The surface

- `SurfaceSite::init` doubles as a cheap probe (`testLandSite`, the relief probe): heavy site data is built by the view after
  `init`, never inside it.
- The frame re-anchors within 50 km: a per-landing set must grow with the explorer.
- Stand far things on the drawn ring's surface, not on DrainageOff 64 m samples, and read them again only when the ring changes.
- Bin a terrain cell by each piece, not all its points (a deep sea's bed spans thousands of rows). Hoist a kind test before any
  per-cell `std::function` call.
- Shore vertices read their neighbours' water with the tide.
- Gate lights on the game's state, not on a camera override (the harness's raised cameras trip it).
- A slope shadows only when steeper than the light; full-signal snow reads as stars. A cloud edge needs ~10 km and a horizon fade;
  heightfields seen from any pitch march per azimuth; expose for the subject.
- Terrain fog sets how far cities show (a haze that thins with height is the user's call).

## Generation: the worlds

- Wide or many planet samples (plates over 200 km, site searches, globe scans) run under `DrainageOff`, or each builds a drainage
  tile. A drainage "lake cell" includes playas and frozen basins: test the water at 64 m before calling it a lake.
- The generation moves no star: companions and comets read the drawn moon orbits. The worlds' dead stars are white dwarfs,
  pulsars and neutron stars.
- Picks stay bounded: `Rng::pick` falls to the last entry on rounding, and pseudo-entries (`STAR_ROGUE`) stay out of every pick
  through `*_DRAWN_COUNT`.
- After a new kind source, check the regress and bench worlds again (Feeyeithis, Wyariothmai, Heilya) and pin a salt if one
  moves. After a kind's first trait, check the per-trait counts: a pick over a kind floods where one trait is its only one.
- Generation never reads the season: the peoples' readers use `thenWorld(g, tmp)`, the weathering `nowKind`. Read the air through
  `bodyHasAir`/`airFrozen`, not `KINDS[].atmosphere`; survey new traits for impossible pairs (aurorae on 40 K deserts).
- Cover materials: large unfaded fields plus finer terms under `matVisible`.
- Compute costly per-body facts only where needed, and time the generator after adding sources. Costs: a 9,261-sector star scan
  2.4 ms; 1,300 system generations 7 ms; a planet sample 1-3.5 µs (2.3 at 64 m with the drainage off).
- Resolve stars with `placeInSector`; only the neighbourhood, the aim and the jumps stay on `starInSector`. A field placed only in
  starless sectors needs an acceptance of d/(1-d) to follow the star density.
- The game's spin is compressed: take a speed's sense and shape from the globe and its magnitude from a hash. The game's days
  last 20 min to 4 h.
- Prefer physics to hand-made variety: condensation on the adiabat gave Sudarsky's classes for free. Look for a closed form
  before planning a worker for a numeric integral (the galaxy's column: 65 ns).
- Gating a hashed rarity by a new share: scale the candidate rate by its inverse and scan the salt again. Work out a dial's reach
  law and simulate it over hundreds of salt layouts before pinning. Rarity survives a longer reach only through per-source power
  (3, 9, 59 and 234 peoples within 200, 400, 800 and 1,600 ly). A rare, strong, far population hashes in coarse cells of its own:
  read a candidate's power before looking for its star.
- Check new rules against the existing data's physics; fix where "which" is decided, then pin again. Check generated magnitudes
  against Earth (a medieval people of 6 billion is wrong).
- Keep `SALT_CELL` and `SALT_DEEP`, so the signals the user found stay; pin again only what must move.
- GCC 9.4, the Linux zips' compiler, builds a constexpr table its own way (B-430). Rows value-initialized at once share their
  arrays' non-zero defaults as one range, and setting an element past a row's first cuts it in place: the rows after the cut
  lose theirs (55 kinds' draw rules, so other worlds on Linux) without a word, and a check reading a lost one crashed it. Set
  every row's arrays first, in order. It also copies a constexpr variable whole at each read (6.6 GB for
  `kindsOk(BUILT_KINDS)`): a `static_assert` reads a table built for it. After a change to how a table is built, compare its
  bytes between the compilers in the container (`build/b430/kdump.cpp`).

## Generation: the peoples

- Peoples live at one star in eight near home: anything that "sometimes" points at one must choose one, not test for one.
- One people's draw that depends on another's breaks "the generator alone": `BodyGen::worldSeed` lets `peopleAgeOf` see a second
  people. A second people's generator (`peopleGen`) is never handed to the planet function.
- Per-cell readers take only what the generator reads; the system's part goes in a second function fed by the star's sector on
  the body; the values a finder reads stay free of the history.
- Never list a world's settlements: the plan ranks places and gives the countryside a density the cell hash reads. Ruin-grid rows
  have their own longitude steps: map rows by longitude (`ruinCellLonIn`), never step cells by index.
- Give each generated item a home that always holds it, and count the coverage in the unit; an empty pool stays empty. Generated
  things take their materials from their world, with no hard-coded fallbacks.
- Ruin layout: test a feature's segment along its whole length (`segmentClear`); a piece against a wall starts at its outer face,
  not its centre line. Street-fronted lots need rectangle (SAT) fits plus each building's colliders against the streets: set back
  once, else drop.
- Scale-aware layouts: cap features sized for small towns, count the houses a grouping places per spot, and size monuments by
  class so the finders get the right radius.
- A `Form&` from `push_back` dies with the next add: copy what is needed first. Break a resting form and keep its supports, not a
  composite under it; grain only on walls of its own material. A new form kind needs its forms-sheet sample and its colliders.
- Buildings: check orientation on per-kind pictures; sunk walls keep a 2.1 m doorway head; tops take the drawn roof's rise; no
  raised floor over records. Judge generators on pictures: per-kind sheets beat whole towns, and variety is measured on
  thumbnails and closest-pair sheets.
- A reach check (each genome variable set in turn, hashing the pieces and colours) finds dead variables in a second.
- History: the order follows causes (lift seasons before the stable sort); events two histories share come from one function, in
  absolute years. Strict coherence checks catch bugs at once; follow main lines past a cap; schedule by year; compare directions
  by dot product, not acos.
- Sky figures belong to the world's sky, not to each chart; a lore that names other peoples reads only their first draw
  (`peopleNameOf`), or it recurses. A locked world's lore says "the day is the year"; the fallen share is 0.6 times the decay;
  "the stage at a year" is the latest begun by its end.
- Works and towns: dams are sought up rivers; valley forms in absolute heights; air lines about 10 m at 2x albedo; frontage for
  about 1,000-1,300 buildings a city. Road webs: a greedy detour spanner over cost-weighted nearest neighbours; city middles get
  their own rule.
- Records: about 1.4 per plan place; archive rows cross the room's middle; decals sit on the wall box's face.
- Instruments: pitch by the shortest lag within 0.92 of the best; one of each per settlement; their kinds are `IPK_` (`PK_` is
  the phones'); a second make leans away from the first.

## The records' text

- Keep each pool uniform (no "the" on some entries, one number) and check article pairs: "in the {season}" once read "in the the
  rains".
- Measure variety as recurring masked frames of 6+ words, formulas out, per people, at 50, 100 and 200 texts, against saved
  texts. Variety comes from a pool's size and composition, not from weights.
- Joins: spaces between parts, local attributes in their own namespace, a number with its noun (`{#n|year}`), tails gated by the
  main part's attribute.
- Tag the lexicon for the anachronism checks (surface words misfire); chronicles need per-year facts.
- Tell events by salience and season, causes first; the reader's close kin first; never a reader's own wedding; routine office
  terms stay private.
- The engine: a capture must not merge into the previous token (`mergeFloor`); pronouns wait until a gender is shown;
  doubled-word checks keep possessives.
- Keep topic weights steady and spread only templates. A share over one world's first fifty texts is a draw: compare populations
  against the old binary.
- A slot drawn a hundred times needs many frames: compose within sentences, and pair tails with their parts (a free verb x object
  x reason made nonsense).
- Descriptions match the medium; a regnal "I" is no pronoun; a second name word is a surname unless it is IVXLC; weight contact
  lines x3-4.
- Peoples differ by their own words in each sentence and by `share=` subsets, not by weights; a generate-and-test frame memory
  turns size into variety (test that it sees the sentence).
- Measure what the data holds before designing on it; keep bookkeeping out of salience and memory, then prove the texts
  byte-identical.
- The user's targets: about 5% repeats at 100 texts, no generic "jars, wells, chiefs", distinct names, every civilisation its own.
  Catalogue dryness 4 cut the repeats at fifty from 41% to 28%.

## Game, screens and sound

- A render path never blocks on asynchronous generation (plans, records, far drainage tiles: B-422's freezes); a landing may.
- Read a key where it is used: a flag set in a screen's update is a frame late for a clock advanced earlier. A per-frame hold that
  re-imposes the attitude takes up turns made elsewhere: compare with the values it last set.
- Long real-time sequences keep closed-form state and a clock-skip hook (`testProbeSkip`), so a replayed frame equals the live one
  bit for bit. Derived probe state is rebuilt by an every-frame ensure step (`probeEnsureClouds`), so loads and swaps keep it.
- A new parking kind is left by every other approach: test the way out, not only the way in. A cross-cutting input change covers
  every screen, the maps included.
- A member named like a free function hides it in every member: name helpers distinctly (`nameOfKey`, not `keyName`).
- Overlay and page states (STAR_MAP, the galaxy page) skip `drawCommonHUD` and the scene: check that statuses show there;
  render-tied work stops while they are open.
- A key-scheme change needs a marker line in the save, not a generation bump.
- The HUD's room: the decoder's column 23 characters; the key line from x 18 to 49; help 51 a line and 21 lines; key labels 15;
  a 320 px line 53 (52 with the margins); the warp line owns y 24 on the right. Never truncate a label or a HUD line: drop a part,
  or place it.
- Map labels need overlap avoidance (dy 0, ±9, ±18) and priorities (YOU and the cursor first); knots stay dimmer than markers;
  markers on lit maps get a dark rim. The cabin computers' screen colours stay at least 0.3 apart in RGB. The landing map's zoom
  never computes tiles.
- The HUD at 1x is green (120, 230, 150), white (230, 235, 240) and amber (240, 190, 90): count them to judge an underline.
- Something to look through (the telescope, a camera) is a camera mode, a feed over the frame with the cabin gone, not a HUD
  overlay. A mystery signal's readout withholds what sent it: the sound carries it.
- Show a count in the other screen's words ("held OF total"), never "left" beside "total", and keep it live: the user reads
  numbers literally. Paring the UI down deletes features outright; it does not hide them.
- Charts: normalise the density to the core's column and weight the arms by light; "N STARS IN 10 LY" counts the 21-sector cube.
  A cabin facing spot reaches where the device shows whole.
- Sound: hold levels by rendering (check the rms after adding effects: a hum and a 0.78 echo stacked to 0.165). Gate new receiver
  effects by the gain (0 at x1): the gain lifts the noise, not the loudness. Keep a meter's level and an overload's drive as two
  readings. A synth peak of exactly 0.80 is the master clamp, which `RenderStats::clip` misses.
- "On the air" is `audio.radio`: a machine programme has no speech or piece pointer.
- A table of `Game`'s methods is a static member declared `const` in the class, defined `constexpr` in its own file (U-02): clang
  reads that definition as an inline variable (weak, emitted only where it is used), GCC as a plain one, so the file reads it out
  of line (`stateHas`) to hold its one definition either way.
- A list of rows whose acts reach `Game`'s protected members (U-03): a `static constexpr` table local to a static method of
  `Game`, its acts captureless lambdas (C++17 makes their pointers constant; their bodies have the method's access). A row's
  optional columns take default member initializers, so clang and GCC 15 let a row leave them out under `-Wextra`. A `const
  double` is no constant expression; a `constexpr` one is.
- The command menu's first tab is the context's name where the context is the explorer's (the ship, on foot, a vehicle, the
  probe, a recording, an instrument played: refined each frame), else the screen's own (U-04): a screen's `padMenu` sets its
  `title` only where its context is the screen's. A trait two others give (a scene: a base in play) stays their conjunction,
  so no row holds one without the other.
- A way back read from a variable folds into a rule only after a check at every close the battery makes (U-05): a scratch build
  printed the two to stderr where they differed (a compare then fails) and wrote every check to a file outside shots/ (`U05_LOG`),
  so the compare stayed the same while it counted them. A variable two screens share is a trap (KI-427): each screen's opener is
  its own (`openedFrom`).
- A harness pass that reads the table covers a new screen with no edit, but it drives only what the rows hold (U-06): the
  controller's commands are a screen's few lines (SPACE's 14 against its 40 keys in the tour's list, the menus none), so the hand
  lists stay as the first pass and the rows run after them, the old outputs unchanged.
- A value every opener must set by hand goes stale where one does not (B-431): the scenes' keys set the scene before they opened a
  screen (Esc, H, J, I, M, Tab; H and C at the title) and the cabin's devices did not, so after the climb aboard their screens
  drew over the ground left behind and saved the explorer back onto it. Taken once where every screen opens (`openScreen`), it
  cannot be forgotten. And one variable cannot hold two things (B-432): the scene a screen is drawn over and the place the title
  resumes were both `returnState`, so the title's own help overwrote the place; the title keeps its game where it was opened from,
  and one reader (`placeNow`) says where the game is.
- A voice's identity comes from strong narrow lines and resonances, not broad noise (compare voices by LTAS distances). Measure a
  medium's surface in the bare render's quiet windows.
- raylib's non-looping music end cuts the last block, and stalls starve its stream. Log each swap's return (the refresh it met);
  hold the swap 3/4 of a refresh before it (2 ms fails under load).
- Auto-explore's turns stay soft, under 60°/s.
