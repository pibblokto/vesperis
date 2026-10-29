# Plan: the frame, faster without changing a pixel (the F series)

Written 2026-10-12 on the user's request ("analyze current code and prepare a thorough, actionable plan on how to
improve performance; improvements shouldn't compromise features, how the game looks, etc.; improvements shouldn't cause
more bugs either or make it more difficult to maintain the code"), from a day of measurements on the dev Mac and an
audit of every subsystem (eleven readers, one per lens, a docs reader for the rules, every finding then re-derived by a
sceptic from the code and the profiles, the nine largest reviewed again for implementation hazards, and a last reader
asked what was missing). 43 findings survived the merge, 41 were confirmed (most with corrections, usually downward),
2 were refuted; they are grouped below into eleven milestones, `F-01` to `F-11`, in the order to build them. The audit's
own last measurement found a bug that comes before all of them: the forest's picture depends on the core count (B-423).

The short version. The frame is already under budget on this machine, so the plan buys headroom for slower machines,
removes the stalls a player feels (a 0.7 MB guide rewritten on every log event, a photo that freezes two frames, the
first frame of a descent), and makes the eight workers earn their keep: today the terrain rings gain nothing from them,
the cabin is drawn by one thread while seven sleep, and the pool's equal bands let four performance cores wait for six
efficiency cores. Every item but one batch is bit-identical (the regress hashes stay); the batch of floating-point
rewrites is blessed once after image diffs. Compiler flags were measured and give nothing worth a line.

## 1. Where it stands, measured (2026-10-12)

Apple M4 (four performance and six efficiency cores), 16 GB, Apple clang 16, the Makefile build (`-O2`), the branch
`release/v1.3.1` as released in 1.3.0. The worker pool (`core/parallel.h`) takes `min(8, cores)` threads and cuts every
pass into equal contiguous bands, one per thread. `bench check` is green with room to spare on this machine; the point of
the measurements is what is left of the 16.7 ms of a 60 Hz frame at the default scale (4x, 1280x800) and how the frame
would fare on a machine with fewer or slower cores, which the serial run stands in for.

### The frame with eight workers and with one (`VESPERIS_THREADS=1`), ms per frame

| row | 8 workers | 1 thread | gain | the sections (8 workers) |
| --- | --- | --- | --- | --- |
| space 4x (the parked world's globe in the window) | 13.6 | 40.6 | 3.0x | the heaviest steady frame at the default scale |
| space 2x | 3.9 | 9.4 | 2.4x | |
| telescope 2x at x1024 | 11.0 | 28.4 | 2.6x | the plate: 193,000 samples of the planet function |
| surface 4x | 8.8 | 17.0 | 1.9x | sky 1.7, warm 0.8, terrain 5.2, objects 0.0, flora 0.1 |
| surface 2x | 6.6 | 9.95 | 1.5x | sky 1.4, warm 0.7, terrain 4.2 |
| surface 1x | 5.1 | 5.2 | 1.0x | |
| forest 4x (the renderer alone, the densest tropical forest) | 14.8 (15.5 standing) | 36.0 | 2.4x | sky 3.6, warm 1.1, terrain 4.0, objects 1.0, flora 3.9 |
| buggy 4x / drone 2x | 11.9 / 7.4 | | | |
| city 2x / capital 2x | 6.1 / 6.2 | | | the forms 2.5 / 0.7 ms |
| descent, first frame | 37 (77 beside a people's capital) | 65 | | the rings' first fill, under the fade |
| probe 2x / belt 2x / vimana 2x | 6.9 / 2.0 / 2.5 | | | |

### The scaling curve, `VESPERIS_THREADS=1,2,4,6,8`

| row | 1 | 2 | 4 | 6 | 8 |
| --- | --- | --- | --- | --- | --- |
| space 4x | 40.6 | 22.6 | 16.0 | 15.3 | 14.0 |
| telescope 2x | 28.4 | 16.3 | 11.5 | 11.7 | 10.7 |
| surface 4x | 17.0 | 12.1 | 8.3 | 9.3 | 8.85 |
| surface 4x, the terrain section | 5.1 | 5.1 | 5.7 | 5.0 | 4.8 |
| surface 1x | 5.2 | 4.4 | 3.8 | 5.25 | 5.4 |

Two facts follow. **The terrain section does not scale at all**: 5.1 ms with one thread, 5.2 with eight, and the surface
frames get slower past four threads (1x: 3.8 ms at four, 5.4 at eight). Its cost hardly depends on the resolution
(4.2 / 4.8 / 5.2 ms at 2x / 3x / 4x), so it is not pixel fill. Every band thread walks every cell of every ring
(`drawTerrainLOD` through `parallelFor(FBH, 32, ...)` in `SurfaceView::render`), and the band cull comes late in the
per-cell body, after the distance test, `cellCovered`, the frustum test with its square root, four `vertexOf`, four
`cache.at` and the water-corner logic; the cells that pass are then set up once more in every band they touch. The
profile agrees: `drawTerrainLOD`'s own time is the hottest symbol of the whole bench with eight workers (5,029 samples
of the run, 1 ms apart), `rasterPolygon` 3,404, `vertexOf` 1,345, `TerrainCache::at` 1,165; serially the same function is 53
samples in 6 s against 408 in 3 s with workers. **Beyond four threads nothing else improves either**: space 4x gains
2 ms from four to eight threads. With equal static bands the six efficiency cores' bands finish last while the four
performance cores wait, and a globe's middle rows hold more pixels than its top and bottom rows.

### Where the time goes

Sampled with `sample` (1 ms), serially so that the work is counted once, and with workers so that the waiting shows.

* **Space 4x (41.9 ms serial).** The globe's per-pixel path is about 60% of the frame: the `drawGlobe` lambda itself
  (304 of ~1,400 main-thread samples in a 2 s window), `atan2` 154, `fmod` 98, `asin` 35, and the four map reads
  `albedoAt` 119, `heightAt` 118, `materialAt` 24, `cloudAt` 12; the mush 125 (two passes of the fine 3x3 box over 1280x800, the
  game's default kernel at 4x; ~3.4 ms serial); the band and nebula pass 89 + `sampleGalaxyBand` 40; `drawSun` 60; `pow` 66.
* **Surface 4x and the forest.** `rasterProjected` + `emitPixel` are ~30% of the serial surface frame and half of
  the forest's (the terrain's and the leaf clusters' pixels), the sky ~25% (its per-pixel `pow` alone 8%: `pow(1 - el, 2.2)`, `pow(cs, 6)`, `pow(cs, 50)`,
  plus `acos`/`atan2`/`asin` for the halo and the pillar, the ring-arc intersection, the cloud grid's bilinear read),
  the mush 7%, `drawTerrainLOD`'s own loop and `vertexOf` small serially and large with workers (above). The
  `pop_heap` the measurer took for a per-frame sort belongs to the drainage tiles' flood at set-up, not to a frame.
* **Telescope 2x.** `gnoise3` 1,616 samples of 2 s: the plate's 8,000 samples a frame of the planet function;
  nothing to gain there but the function's own cost, which is generation and out of scope.
* **Compiler flags give nothing.** Scratch builds of the same tree benchmarked back to back: `-O3`, `-O2 -flto=thin`
  and `-O3 -flto=thin` all stay within noise of `-O2` (space 4x 14.0 → 12.8-13.9, telescope 10.7 → 9.9-10.5, surface 4x
  8.85 → 8.6-8.9) and keep all 22 regress hashes. Thin LTO may be worth ~5% on the space rows; nothing else here.
* **What the bench cannot see.** The real game's `platformPresent` (a 1280x800 RGBA frame uploaded to the GPU every
  frame, the post shader), `platformAudioPump` (the synth on the main thread) and input polling are not measured by
  the headless harness. They are the first thing to instrument (F-01).

### A bug found on the way: the forest's picture depends on the core count

B-310's contract says a frame drawn by the workers is the serial loop's. The critic asked for the one measurement the
session lacked, and it fails for the flora: `scene felisian_forest` with `VESPERIS_THREADS=1` against the default
eight workers differs by 13,921 pixels at 4x (1.36% of the frame), 3,419 at 2x, 506 at 1x, with channel steps up to 130
(trees shown in one run and hidden in the other, in the canopy rows of the middle fifths of the frame); the shore scene
is byte-identical at both scales and the serial `regress` passes 22/22. The cause is in `flora.cpp:264, 304-306,
336-340, 393-402`: the canopy-coverage grid is allocated per band and marked only by the clusters that pass the band
test, while `covered()` reads every tile under a canopy, so a far tree whose canopy crosses a band edge is hidden in one
layout and drawn in another. The band count follows the thread count (six at 1x, eight at 2x-4x on this machine), so a
four-core machine draws a different forest than this one. The frames look alike to the eye; the contract is broken all
the same, and every band-boundary change below (F-05, F-06) would otherwise be unprovable for the flora. It is B-423 and
the first thing built.

The raw numbers of the session (bench runs, profiles, the flag builds) are summarised in section 10.

## 2. Targets (so that "faster" can be measured)

The default picture is 4x; a 60 Hz frame is 16.7 ms. Every row is `bench check` on the dev Mac at rest, the median of
three runs; the one-thread column (`VESPERIS_THREADS=1`) stands in for a machine with few cores and for the web build.
The targets are the verified gains of section 4 added up with a third held back for overlap and optimism (the sceptics'
corrected numbers, never the finders'); a milestone that misses two thirds of its own estimate is reconsidered before the
next starts (section 6).

| row | now, 8 workers | target | now, 1 thread | target | what gets it there |
| --- | --- | --- | --- | --- | --- |
| space 4x (a globe in the window) | 13.6 | **7** | 40.6 | 30 | F-05 cabin, F-06 dealing, F-07 globe trims and wrap, F-10 |
| surface 4x | 8.8 | **4.5** | 17.0 | 12 | F-04 terrain, F-06, F-07 mush and sun, F-10 sky |
| forest 4x (the densest scene) | 14.8 | **10** | 36.0 | 28 | F-04, F-05 grids and reflections, F-06, F-07, F-10 |
| buggy 4x (the nose camera) | 11.9 | **8** | | | F-04, F-05 feeds |
| telescope 2x at x1024 | 11.0 | **8** | 28.4 | 24 | F-06 dealing and plate, F-07, F-10 |
| surface 2x | 6.6 | **4** | 9.95 | 7 | F-04, F-07 |
| landing map at 4x (no row yet; estimated 17-18) | over budget | **under 14** | | | F-01 row, F-05 blits |
| descent, first frame (home world / beside a capital) | 37 / 77 | **20 / 55** | 65 | 40 | F-03 |
| a frame in play on a sector crossing, a sighting, a mark, a photo, the five-minute autosave | +20-35 ms (a 0.7 MB guide written) | **+3** | | | F-02 |
| a photo at 4x | +17 ms encode, +25 guide | **+1** | | | F-02 |
| recording at 4x in space | 33 ms frames | **16.7** | | | F-02 |
| a frame in a capital on grass (listing the trees) | +20-120 ms (estimated) | **+1** | | | F-02 |
| sprint start or stop at 4x | six frames +2.3 ms | +0.5 | | | F-02 |
| the landing map's zoom window on a wet world, a map not yet generated at the map's opening | freezes of seconds / 170 ms (unmeasured) | a progressive fill, no freeze (the user's call) | | | F-01 rows, F-02 |

The rows that matter most for the player are the first three (the steady frame at the default scale on the heaviest
scenes), then the stalls, which the bench cannot see today and which F-01 makes measurable. Nothing in the picture
changes: every target holds with the 22 regress hashes unchanged, except F-10's batch, blessed once after image diffs.

## 3. The rules every item respects

From `docs/decisions/log/`, `02-rendering.md`, `06-surface.md`, `08-testing-and-tools.md` and `KNOWN-ISSUES.md` (read
before touching any of this):

1. **The pixels are the serial loop's.** A frame drawn by the workers is bit-identical to the same frame with
   `VESPERIS_THREADS=1` (B-310). Parallelism is bands of rows: every band thread walks the primitives in the serial
   order and `setRasterBand` clips its fills to its rows, so each pixel's write order never changes. Dealing out the
   rows differently (finer bands, dynamic chunks) keeps this, because a pixel's value depends on its row's walk and not
   on which thread performs it; binning the terrain cells by the rows they can reach keeps it too, because a band
   still draws, in order, every primitive that can touch its rows. Splitting primitives among threads was rejected (a
   race on the depth test) and stays rejected.
2. **A frame never depends on a worker's timing.** Work a frame needs is built in slices on the pool inside the frame
   (the plate's 8,000 samples, the band's keyframes, the far scan), never left to a thread of its own; background
   threads (the drainage tiles, the plans, the texts, the ahead worker) fill caches that the main thread copies, and
   the picture is the same whether the cache was warm or filled on demand.
3. **No generation change.** `sampleSurface` and everything hashed from the seeds return the same values at every
   detail; a change to the noise or the planet function must be bit-identical (GEN_VERSION 13 holds, the five systems,
   ten maps and three profiles of the regress stay).
4. **Double precision everywhere**, as decided (the probe's single-precision cloud noise is the one recorded exception,
   a per-frame visual outside the generation).
5. **The look is fixed.** The 16-bit pixel, the ramps, `toRGB`'s interpolation and dither, the mush's definition
   (k x k box, bank from the row below, two passes, three on venusian worlds), the HUD after the mush in RGB, the
   resolution, the counts of trees and buildings. Rewrites of floating-point arithmetic that move the regress hashes
   (`pow` as multiplications, reassociation) are allowed only in F-10's batch, reviewed on the images and blessed once
   with the reason recorded.
6. **One code path.** No fast variant, no toggle that forks the rendering, no prototype left in the tree; experiments
   live in a scratch copy and are removed once the decision is recorded.
7. **Workers call pure functions.** The caches and the framebuffer are written by the main thread or by exclusive
   rows; no locks in the render path; `WorkerPool::run` is not reentrant.
8. **The B-422 rule.** Nothing far off is computed on the main thread for a glance; the landing map never builds a
   drainage tile on the main thread.
9. **Portable C++17** for the six release targets, warnings as errors in `core` and `galaxy`; no raylib in game code.
10. **The regress is narrow**: four frames at 1x, nothing at 2x-4x, nothing of the cabin, the forms, the flora, the
    screens. Every item that touches those is proved by an A/B of the harness frames (`pngdiff`, F-01) and, for a new
    band-parallel pass, by the unit check that draws the frame serially and with the workers in one run
    (`parallelThreadsOverride()`, A-02's template).
11. **Per milestone, in order, nothing committed unless asked**; the budgets stay green on every row.
## 4. The findings, verified, by milestone

Every item names what the code does today (with the lines as of `release/v1.3.1`), the change, the gain the sceptic
could defend (ms per frame at 4x on the named bench rows, or ms per event), the pixel effect with its proof, and the
hazards the reviewers found. "Identical" means the same arithmetic in the same order, pure work skipping, or only a
change of which thread computes a pixel; such items keep every regress hash. Gains do not add across items that touch
the same pass (noted where it matters).

### F-01. Measure what the bench cannot see; the proof tools (S-M, nothing drawn changes)

Everything after this is judged against it. The bench measures the headless frame; the player's frame also pays the
window, the audio and the files, and the regress hashes four 1x frames, nothing at 2x-4x, nothing of the cabin, the
forms, the flora, the feeds or the screens.

* **The real game's frame sections** (`main_raylib.cpp`, `platform_raylib.cpp`): under `VESPERIS_TRACE=1` time
  `platformAudioPump` (the synth block), `UpdateTexture` and `EndDrawing` separately (the swap's vsync wait must be its
  own number), the screenshot and recorder calls, and print max and mean once a second to stderr. The getenv is read
  once (the X-02 lesson). This decides F-08 and F-02's recorder work.
* **`pngdiff`** as a harness mode: two PNGs in, the count of differing pixels, the largest intensity step, the first
  differing coordinate, an optional diff image out. The docs name it; it does not exist in the tree.
* **Serial-versus-workers checks in `unit`** on the frames the regress never hashes, by the A-02 template
  (`parallelThreadsOverride()`, `main_headless.cpp:6340-6361`): the cabin frame (light on and off, depolarised, on the
  roof, drones out), the forest frame, the nose-camera and radar feeds, a shore frame at 4x, a landing-map and a
  sector-map frame. The forest check was run by hand during the audit and fails today (section 1, B-423): it enters
  `unit` with B-423's fix and guards it from then on.
* **A per-band trace for the terrain and the flora** (`VESPERIS_TRACE`): the ms of each band's walk, water set-up and
  fill, so F-04's replicated part is read before it is removed and the horizon band's skew is seen.
* **Micro-benches printed by `unit`**, not checked: an empty `parallelFor(8)` dispatch and fifteen back to back (µs
  each; decides F-06's spin); `bench noise`: 1e6 calls each of `gnoise3`, `gnoise3d`, `worley3` and a `visitFeatures`
  caller, ns per call and a bitwise equality count against the current code (decides F-07's gradient table).
* **Bench rows without budgets** for the states the bench lacks: the landing map at 4x with the cursor moving, a
  sun-facing surface frame at 4x (the veil's disc), the radar camera at 4x, `Guide::save` of the player's own 0.7 MB
  guide (a copy of `guide.txt` in `shots/tests/`, or a scratch program), a recorded frame at 4x. Printed, so a later
  change that gives the gain back is seen.
* **The measurements the critic found missing**, each a row or a trace, none a change: the landing map's zoom build
  split into samples, roads, plan edges and works (`zoom` mode on the home site, a big wet world, a people's world); a
  count of blocking `mapFor` calls and an arrival's frame times while eleven map threads generate; the telescope, the
  probe (and its 4.2 s handover), the city, the capital and the decoder at 4x (no row above 2x exists for them, and the
  forms' fill and the globe tracer scale with the pixels: they may be the real worst steady frames); the real game's
  frame-time histogram (p50, p99, max over two minutes) parked with the player on, with the radar, on a windy surface,
  driving, in the decoder, beside the present split; the runnable threads per state in the first minute after a landing
  (B-422's method); RSS after 30-60 minutes at 4x and the guide's load time at start; the forest rows at the game's
  kernel (`bench forest` and the scene and sheet modes leave `setFramebufferScale`'s k = 5 while the game's fine setting
  is k = 3 at 4x, so the forest budget row measures a kernel the game never runs); one bench and one
  serial-versus-workers hash on Linux and Windows (none exists; KI-014); a sweep of the thread cap and `minPerThread` at
  1x-2x (the globe runs on two threads at 1x and five at 2x by the `n / minPerThread` rule); `roadsMs` per rebuild while
  driving on a people's world and the comet re-anchor's init, tiles and fill split (KI-302).
* **The flat-core reading of the scaling curve** stays in the docs (`08-testing-and-tools.md`): `VESPERIS_THREADS=1,2,4,6,8`
  is the cheapest diagnosis of a pass that does not scale.
* **Done 2026-10-12** (`docs/decisions/log/4-1.3.1.md` F-01; the numbers in `08-testing-and-tools.md`). What the measurements decide for the items
  below. F-02: the guide's save is 7.9 ms (not 20-35: a third of the gain, kept, as it still drops a frame at space 4x); a 4x PNG
  16-19 ms and a recorded frame 27.8 ms (as estimated); the zoom builds in 24-32 ms (not seconds: its progressive fill is not needed);
  a world not yet mapped blocks the landing map's opening 287-414 ms (`mapIfReady` kept); the trees listed against a settlement's
  buildings 1.0 ms at the one test capital in a forest (the 20-120 ms stays an estimate: no test world has a big capital in a forest);
  a sprint's direction-table rebuilds +1.6-1.8 ms on one to four frames at 4x. F-03: a comet's re-anchor costs its next frame 30-33 ms;
  the people's descent sits on its 80 ms budget (77.8 ms median, a quarter of the runs over). F-04: every band walks every cell and
  sets up every wet cell's water (the forest: 9,079 and 2,982 in each of eight bands; surface 4x: every band 2.5-5.2 ms whatever its
  rows). F-05: the landing map at 4x is 7.3 ms and the radar camera 6.7 ms, under budget (the blits and the feeds are gains, not
  rescues); facing the sun costs 2.1 ms. F-06: the pool's dispatch is 34-50 µs after a rest (the spin stays); ten threads gain 0.6 ms
  on space 4x while the surface rows are fastest at four; half the rows-per-thread rule gains 0.36 ms on space 2x. F-07: `gnoise3`
  88 ns a call against `gnoise3d`'s 23.5 (the gradient table worth more than estimated). F-08: the synth's block is 0.76 ms at most and
  the upload 0.4-0.5 ms, so the helper thread is dropped and no unpack ring is planned; the pacing becomes the item (`SetTargetFPS(60)`
  on the vsync: 4-28% of the window's frames over 17.5 ms with 7-13 ms of work, KI-407). New over budget at 4x: a globe filling the
  frame (the telescope 25 ms, the probe's fall 26 ms) and the decoder with a piece playing (18.5 ms; KI-406). Not taken: Linux (Docker
  did not answer) and Windows (no machine).

### B-423. The forest's picture depends on the core count (S, a bug; one of the two pictures changes)

Found by the audit's own measurement (section 1). Two ways to restore B-310's contract for the flora:

1. **Pin the flora's band layout** (recommended first): the flora's bands become a constant per scale, `FLORA_BANDS =
   min(8, FBH / 32)` (six at 1x, eight at 2x-4x: today's layout on this machine), run through a `parallelBands(n,
   bands, fn)` primitive that deals exactly that many chunks and, with one thread, runs them sequentially in order. The
   serial loop then is the banded loop, the picture no longer depends on the thread count, and nothing changes on this
   machine or in the regress (its three surface scenes hash the same both ways today); the one-thread picture changes to
   match. The per-band grid stays what it is: an approximation of the canopy cull that a band edge can cross, now a
   deterministic one. ~S.
2. **One coverage grid per frame by the serial rule**: fill it on the main thread from the tree list near to far before
   the bands, every band reading it. The depth-tested speck marks cannot be reproduced before drawing, so the rule
   changes slightly, a few far trees that show through canopy holes today are hidden, and the picture on this machine
   moves: a bless of the forest frames (they are not in the regress; `cmp` against the previous binary documents the
   change) and a serial pass of ~0.2 ms. ~M.

The user chooses the reference picture (the rule says the serial loop's; the eye cannot tell the two apart). Either way:
a `docs/decisions/log/` row, the forest frame in `unit`'s both-ways check at 1x, 2x and 4x, and the `bench forest` line's
"visits over the bands" keeps its meaning. B-423 precedes F-05 and F-06.

### F-02. The stalls in play (S-M, identical, per event)

Nothing here moves a bench row; all of it is felt. The guide and the PNGs are the two the bench never sees because its
guide is fresh and it writes PNGs outside its timed loops.

* **`Guide::save` on every log event** (`guide.cpp:127-167`, `guide_screens.cpp:176-179`, 24 call sites). Every sector
  crossing, sighting, grave, road, landing, mark, radar lock, photo and the 300 s autosave rewrite the whole guide on
  the main thread inside the frame: the player's `guide.txt` is 731,810 bytes, of which 566,561 are five `probecam`
  lines holding 39,615 doubles formatted at precision 12 (~0.4-0.8 µs each), 113 KB the log. Measured by composition:
  20-35 ms per save, one or two dropped frames per event, growing with every probe kept; zero for a fresh guide. The
  landing's Enter saves in the black frame at fade 0 (a delay, not a stutter); the Q ascent, the events and the photos
  show it. **Change** (the sceptic's, simpler than the finder's second file): a `ProbeRecord` keeps its four list lines
  (`probecam`, `probetime`, `probeprofile`, `probelayers`) as one `std::string` built by `probeLines()` where the record
  is made (`probeKeep`, `importInbox`'s copy, `Guide::load` after parsing), and `save` writes `f << r.lines` after the
  `probe` header line, whose lent/saved fields do change. Same bytes, one file, no migration, the inbox exchange
  unchanged. Then, only if a measured burst remains (LANDING and SECTOR log in one frame today), a `guideDirty` flag
  saved once at `Game::frame`'s end, keeping the direct save where the harness reads the file back. No writer thread.
  **Gain** ~25-35 ms → ~1.5-3 ms per event. **Proof**: `cmp` of the saved file before and after (byte-identical),
  `unit` (X-04 recording and inbox checks), `flow`'s guide-reloaded line; time `Guide::save` x20 on `guide.txt` in a
  scratch program before building (the hitch is unmeasured). **Hazard**: the cached text must be rebuilt wherever the
  four vectors are set; today only the three places above.
* **The PNG encoder, same bytes faster** (`core/png.cpp:89-128`, `:18-35`). `writePNG` packs rows with three
  `push_back` per pixel (3.07 M at 1280x800), runs Adler-32 with two `% 65521` per byte in a dependent chain, copies
  the 3 MB stream twice and CRCs it bytewise. Calibrated on the bench's few PNGs at ~6 ms per MB: 10-25 ms per 4x image,
  ~17 typical (KI-022's 2-8 ms at 2x agrees). **Change**: `raw.resize` and pointer stores; Adler in runs of 5,552 bytes
  with one modulo per run per sum (zlib's NMAX: identical by construction); `z.reserve`; the chunk written as
  length+type, data, CRC with the running `crc()` over type then data and no copy. Keep the bytewise CRC. **Gain** ~17 →
  ~6-8 ms per 4x image, ~30 ms off a panorama; the harness's hundreds of 1x PNGs a little faster. **Proof**: `cmp` every
  PNG `flow`, `bench` and `unit` write against the previous binary's (hundreds of files, byte-identical); the gallery
  reads them back through `readPNG`.
* **The PNG writer off the main thread** (`main_raylib.cpp:70-85`, `game.cpp:233-237`, `probe.cpp:977`). A photo at 4x
  still stalls ~7 ms after the encoder fix, and recording (`Ctrl+R`, one PNG per frame) in space at 4x makes 33 ms
  frames (13.6 + ~1 present + the encode > 16.7). **Change**: `writePNGAsync(path, px, w, h)` and `pngFlush()` beside
  `writePNG` in `core/png.cpp`: a bounded queue (depth 3, 4 MB copies, the producer blocks when full so no frame is
  dropped and the names stay in order), one `std::thread` calling the same encoder; `readPNG` begins with `pngFlush()`
  (a photo is read back only through it), `Game::openGallery` flushes (the gallery scans the directory), both mains
  `atexit(pngFlush)`; `recordFrame`, the screenshot and `probeKeep` use it; the panorama and the harness keep the
  synchronous `writePNG`; Emscripten keeps it synchronous (a fifth pthread beyond `PTHREAD_POOL_SIZE=4` could stall).
  **Gain** the main-thread stall ~7 → ~0.4 ms per photo or recorded frame at 4x; recording in space back to the 16.7 ms
  cadence. **Hazards**: the three lifetime rules (flush before exit, before a take directory is reused, before a PNG is
  read back); an efficiency core encoding 7-15 ms per frame may fall behind 60 fps and the bounded queue then blocks,
  which is today's cadence, no worse. Record the decision (a thread for a result no frame waits for, like the maps and
  tiles).
* **The trees listed against every building of a settlement** (`flora.cpp:84-108`, `surface_view.cpp:579-589`). For
  every candidate tree within reach + 40 m of a settlement, `forTrees` rotates it into every building's frame with a
  sincos before the rectangle test, and `inSettlementBuilding` does the same for logs, rocks and colliders. No bench
  row stands in a settlement with trees, but the arithmetic is plain: a capital's core holds up to ~1,000 buildings
  (KI-378), the listing radius is 24 cells, grass gives ~1 candidate per cell: ~900 trees x 900 buildings x ~25 ns =
  ~20 ms per frame, 60-120 ms in a forest over a capital; the plan places capitals on grass by preference. **Change**:
  before the rotation, `if (ex*ex + ez*ez > ((hw+m)^2 + (hd+m)^2) * (1 + 1e-9)) continue;` with the same margin the
  rectangle test uses (a rotation preserves length, so nothing the test would keep is skipped); move the yard test
  (`flora.cpp:101-105`, one hash, discards 67-95%) before the building loop. No sin/cos cache (the one way to differ by
  an ulp). **Gain** 0 on the bench; tens of ms per frame in a capital, every frame. **Proof**: hash the tree, log and
  rock lists at a capital before and after in a scratch program (identical), time the listing; `unit` (walls, forms),
  `regress`, `cmp` of `bench_capital.png`.
* **The sky direction table rebuilt six times per sprint** (`surface_view.cpp:105-113, 3604-3607`). The sprint's FOV
  kick moves `proj.f` by 7.1% over 0.6 s and `render` rebuilds the 1,024,000-entry LUT (24.6 MB) every 1.2% of that:
  six serial rebuilds of ~2.3 ms each at 4x per sprint start and stop (the finder's 10-12 ms was 4-5x too high: the
  bench's own `applySettings` rebuilds calibrate it at ~2.2 ns per entry). **Change**: `parallelFor(FBH, 16, ...)` round
  the row loop; the 1.2% rule stays (hysteresis would change which frames rebuild and so the sky during the ramp).
  **Gain** ~2.3 → ~0.5 ms per rebuild; never a dropped frame on this Mac, 7-10 ms off frames already slow on a 3-5x
  slower machine. **Proof**: a sprint at 4x dumped and `cmp`-ed against the previous binary and against
  `VESPERIS_THREADS=1`.
* **The star neighbourhood rebuilt every frame of a Vimana flight** (`starfield.cpp:448-459`, `autopilot.cpp:225`).
  `StarNeighborhood::update` rebuilds the 21^3 cube (9,261 `starInSector`, ~1.4 ms at home) whenever the ship's sector
  changed; a 10,000 ly flight crosses 3.7 sectors a frame, so every frame rebuilds. **Change**: the old list is already
  sorted lexicographically (the loops nest ascending); walk the new cube in the same order with one forward cursor over
  the old vector, copying the entry when the sector matches, calling `starInSector` only outside the old bounds. Same
  stars in the same order (a `Star` is a pure function of its sector). **Gain** ~1.0-1.5 ms per flight frame (vimana 2x
  2.5 → ~1.2), the spike per sector crossing on short flights gone; nothing outside flights. **Proof**: a unit check
  walking 50 positions coreward, incremental list == fresh list at each step, including a far jump; `cmp` of
  `bench_vimana.png` and the flow's flight frames.
* **The landing map's probe site re-initialised per cursor step** (`landing_map.cpp:548-551`, `site.cpp:557-582`).
  Every cursor move (every frame with Shift held) runs `SurfaceSite::init`, which fills five terrain caches (106,496
  vertices, 3.8 MB) the probe never reads. **Change**: split `init` into `initFrame` (season, gen, radius, gravity, the
  tangent frame, levels, ring profile) and `init` = `initFrame` + the five cache inits; the probe, `testReliefClass`
  and `testLandSite` call `initFrame`. **Gain** ~0.3-0.5 ms per frame while Shift-scrolling. **Hazard**: a cache-less
  site handed to the drawing would index an empty vector; the name and a comment say what it is for. Fold into the
  landing-map work of F-05 if preferred.

* **Three loading moments the critic found that no finding covered** (each changes when something appears, never a
  pixel; whether a progressive fill is wanted is the user's call, so they are measured in F-01 and decided then):
  the landing map's zoom window (`landing_map.cpp:200-245`: 256 x 128 samples at 64-512 m on the pool, then the roads,
  the plan's edges and places; rebuilt at `Z`, when the cursor leaves 75% of the window and again when its tiles arrive)
  freezes the main thread for the order of seconds on a wet world, more than any hitch above: build it the plate's way
  (blocks within a per-frame budget, the whole-world map standing in) or on a thread read through an IfReady accessor;
  `mapFor` (`space_view.cpp:183-190`) generates a planet map synchronously (168 ms) at the landing map's opening or in
  the drone bay when the map is not there, and eleven generator threads compete with the frame at an arrival: read both
  screens through `mapIfReady` with the existing READING THE WORLD state until the map exists, and bound the
  generators to two at a time, nearest first; the comet's re-anchor every 0.35 radii (KI-302) and the roads' block
  rebuild every two ruin cells on a peopled world (`surface_view.cpp:1215-1223`) freeze the frame while driving or
  flying: prefetch the next anchor's tiles when within 0.3 radii (off the frame, bit-identical) and walk the roads'
  block in slices if `roadsMs` reads tens of milliseconds.

* **Done 2026-10-13** (`docs/decisions/log/4-1.3.1.md` F-02). The guide's save 7.6 → 1.0 ms (the player's own 731,810 bytes 7.0 → 0.9; the load +7 ms
  at the start, where the lines are made); a 4x photo's PNG 15.4 → 7.2 ms; recording at 4x 24.5 → 9.6 ms a frame (the recorder's part
  15.8 → 0.1: the writer's thread); the trees at the prehistoric capital in its forest 0.99 → 0.70 ms a frame and the descent beside the
  modern capital 78.4 → 75.1 ms (no test world has a big capital in a forest: the 20-120 ms stays an estimate); the sprint's slow frames
  gone (2-4 over the median + 1.5 ms → 0); Vimana 2x 2.57 → 2.24 ms (a third of the estimate: the cube costs 0.5 ms along the bench's
  flight, not 1.4); the landing probe's `initFrame` within noise (the five caches' fill was cheap; `load map`'s p99 -0.6 ms). The landing
  map and the drone bay open at once on a world not mapped yet (the user's choice of the two ways asked; F-01's 287-414 ms were the
  harness's parking: two maps, the first for the last system's season, measured again 358-420 ms, none now). Measured and not built:
  the guide's dirty flag (two saves in a frame cost 2 ms now), the arrival's map threads two at a time (the first second 8.6 ms a frame
  against 8.4-8.8 after: nothing to win on ten cores), the roads' block in slices (`load roads`: 0.0-0.3 ms a rebuild; the hop's 26-29 ms
  frame is the rings' refill after a 4 km move, F-03's case), the comet's re-anchor (F-03's).

### F-03. The first frame of a descent, and the tiles (S-M, identical, per event)

* **The rings' first fill, serial** (`surface_view.cpp:2213-2225`, `site.cpp:23-30`, `game_states.cpp:52`,
  `landing_map.cpp:107-112`). At the landing's Enter, `SurfaceView::init` and the first render share one frame; the
  camera at 1800 m draws lod1 (r 36) and lod2/lod3 (r 26): ~9,500 cells, every one a miss, sampled serially by
  `warmTerrainLOD`'s fill loop while the pool idles (~2-3 µs each: 19-28 ms of the 37 ms frame; the scaling curve
  64.7/46.8/39.1/40.7/39.5 ms at 1/2/4/6/8 threads shows the ~30 ms floor). A re-anchor every 50 km refills all five
  rings (~17,000 cells) the same way; the buggy at speed pays the ahead worker's residual misses here. **Change**: the
  serial disc pass only counts misses (the tag test) and collects the materials seen; when the misses exceed ~64,
  `parallelFor` over the disc's rows of cells, each worker calling `cache.at` for its rows (writes disjoint by row;
  distinct cells of a ring map to distinct torus slots since radius + 2 is under half the torus for every ring); after
  the join, `mesoFor` per material seen, serially; the same body in `prefetch(budgetMs)` per ring; a unit assert
  `radius + 2 < (1 << logN) / 2 - 1` for the five rings. Amend M7-01's rule in `docs/decisions/log/` and `06-surface.md`:
  the caches are written by the main thread or by the pool inside a joined region on disjoint slots; the asynchronous
  ahead worker keeps its staging buffer. **Gain** descent first frame 37 → ~18-22 ms; beside a capital 77 → ~58-62; a
  re-anchor ~-30 ms; the buggy -0.1-0.3 ms per frame at speed. **Pixels**: a cell computed on a worker holds the bits
  on-demand sampling would (a pure function of the cell and the site's state, frozen within the frame); the thread-local
  drainage state is a cache of pure functions. **Hazards**: a cold landing (KI-338/403) could make eight workers hit
  different missing drainage tiles at once, each `buildTile` spawning its own samplers; measure a cold landing before
  deciding whether to bound it; `prefetch(3.0)`'s budget may overshoot by one ring. **Proof**: `VESPERIS_TRACE`'s
  `warm NNNN m: N misses, fill X ms` lines before and after; `regress`; `unit` (ring seams 0.0000 m); `capsule` 0.00%
  holes; a landing scene at 4x `cmp`-ed serial against workers; the descent rows in alternating pairs.
* **The drainage tile's serial tail** (`drainage.cpp:356-367`, `:312`). After the parallel sampling (176,400 samples
  at 240 m on eight threads, ~50 ms wall), `buildTile` runs its flood, breaches, directions and a full `std::sort` of
  176k cell indices by `(hc desc, popIndex desc)` with two indirect loads per comparison (3 M comparator calls, 6-13 ms)
  on the calling thread; the breach loop empties its Dijkstra queue one `pop` at a time. **Change**: sort packed
  `uint64` keys instead (the monotone bits of `hc`, with `-0.0` canonicalised to `+0.0` as the comparator treats them
  equal, inverted, over `~popIndex`): the comparator is a strict total order so the permutation is the same one, hence
  `topo`, `acc` and `lvl` bit-identical; swap the queue with an empty one. **Not** the finder's post-order walk (the
  sceptic showed it is a generation change: at a breached basin the pond cells' accumulation reaches the rim cell after
  it has propagated and is lost downstream today; a walk would carry it) and **not** the sampler re-sizing (B-422
  measured and refused that sharing). **Gain** -6 to -12 ms per tile; a landing's synchronous prefetch waits one lane,
  so the same per landing; the rare main-thread tile 100-200 → 90-190 ms. **Proof**: a scratch program dumps a tile's
  `lvl/acc/dir/up/flag` arrays before and after and `cmp`s them (a differing byte is a key-mapping bug, never a
  tolerance); `regress` (the site profiles read the drainage); `unit`'s drainage statistics; the `drainage tile ...
  built in N ms` trace lines. A quirk found on the way (the breached basin's catchment) is a generation question for
  the next GEN bump, not for this plan.
* F-07's noise work (the gradient table) takes another ~5-12 ms of wall off a tile's sampling and 3-8 ms off the
  first frame; counted there.
* **Done 2026-10-13** (`docs/decisions/log/4-1.3.1.md` F-03). The descent's first frame 40.0 → 22.3 ms at the home world's site (the estimate 20-22)
  and 78.9 → 45.5 beside the modern capital (the estimate 55-62): the rings' misses on the pool (the fill 13.9 → 3.5 ms), the missing
  meso tiles a material to a worker, the landing's three ground textures on the pool (7.0 → 1.8). A comet's re-anchor's next frame at
  4x 30-33 → 15-16 ms, a 4 km hop's 27-29 → 12-15. The tile: the sort as integer keys (6 → 2 ms) and the queue's clear as planned, then
  what the tile's own timing found larger: every breach walk that reached its guard had turned into two cells sinking in turn 5 mm a
  step (520 million steps, 3.1 s, in a tile of lake country), now computed a binade at a time (`drainageSink`), and the routing noise
  made by the sampling threads: `drain`'s median tile 163 → 96 ms, its worst 3,202 → 177, every tile's hash the same. Not identical in
  one respect, by the user's choice: the fill on the pool exposed that a drainage sample depended on its thread's order (the cached
  neighbourhood was bent with the first point's latitude and kept the first point's coarse scale); the neighbourhood is now made from
  its cell, and five of 67 scenes moved once by 1-45 pixels (one or two steps), the regress unchanged, no GEN bump. Left: the prefetch
  during the descent serial (as planned), the galaxy band (F-06), the capital's town marks (11 ms) and KI-408 (a generation question).
  The timing batch on mains power (2026-10-14, four alternating pairs against F-02's binary): the space rows and the telescope within 1%
  (space 4x 14.09 against 13.97 ms), the battery batch's 20-33% gone; the descents 39.8 → 23.2 and 77.7 → 47.4 ms.

### F-04. The terrain rings (M-L, identical, the surface frame)

The largest single lever on the surface. Today every band thread walks every cell of every ring (`render`'s
`parallelFor(FBH, 32, ...)` calling `drawTerrainLOD` for the five rings, `surface_view.cpp:3656-3667`): ~16,000 cells in
the discs, the band cull at `2325-2334` after the distance test, `cellCovered`, a square-root frustum test, four
`vertexOf` (each a `TerrainCache::at` load before the stamp compare), four `cache.at` and the water-corner logic, and
the cull exempts wet cells and edge cells, so every wet cell's water quad is set up in all eight bands (wind sincos,
four corners of normalize, pow 28, pow 4, two sin, exp, then `rasterPolygon`'s clip and projection before the rows are
rejected). The sceptic confirmed it two ways: the serial 2x run (3.1 ms) against the parallel one (4.2) fits only if
~2.2-2.4 ms of the resolution-independent 2.43 ms is replicated per band (the pixel fill is 0.167 ms x scale² at every
scale), and in the forest the main thread's nearly empty band 0 costs 1.4 of the section's 4.0 ms. In the whole-bench profile
`drawTerrainLOD`'s subtree is 22,245 samples: the water quads ~54%, the walk 34%, the fill 9%. Build the three items in
this order: the water corners per vertex first (the bin's hull then reads cached corner positions, and the quads that
the cull never removes stop being replicated), the warm pass's rows, then the binning.

* **Bin the cells by band once per frame** (`drawTerrainLOD` split into a bin pass and `drawTerrainCells`). After the
  warm, one serial pass walks rings 0..4, rows, columns in the serial order, evaluates the three culls once (the
  `camPos / cs` divisions hoisted per ring), reads the four `VtxCache` corners, computes `wet/cut/wCorner` as `2316-2327`
  does, and takes the cell's rows as `[ceil(minSy - 0.5) - 1, floor(maxSy - 0.5) + 1]` over the corners' `sy`, the
  corners raised by `SHORE_LIP` for cut cells, the water corners for wet cells and the wall bottoms for the outermost
  ring's edge cells, all through the one `project()` inline exposed from `raster.h` (the ±1 slack covers the ulp between
  `vc.sy`'s `f*y/z` and `project`'s `f*y*(1/z)`); a corner behind `NEAR_Z` or a foam cell (`cdist < 90`, `cs <= 16`)
  goes to every band. Each intersecting band gets `{ring, cx, cz, flags, wCorner[4]}` appended; the band threads draw
  their lists with the ring's `RasterParams`, no culls, the raster still clipping to the band. **Why identical**: pure
  work skipping and a change of when the cull runs; each band draws, in ring/row/column order, a superset of the cells
  that can touch its rows, so each pixel's write sequence is the serial loop's (B-310's contract holds). **Gain** the
  terrain section 5.2 → ~2.3-3.7 ms at 4x (~2.0; the range is which core takes the horizon band), 4.2 → ~2.0 at 2x,
  forest 4.0 → ~2-3 (its cost is mostly row skew, below); the 4-to-8-thread slowdown of the 1x rows goes. A bonus on
  flat far ground: far cells projecting into 0.3 px hold no pixel centre and drop with their water quads. **Hazards**:
  a missed row at a band edge shows as a one-row hole in parallel only (the `THREADS=1` comparison at 4x catches it; the
  1x regress does not); the bin is a serial ~0.6 ms on the main thread; reuse the lists' vectors; `VESPERIS_SKIP=wq`
  must keep working; a stale stamp in the bin pass means a vertex the warm did not compute (assert it in the harness).
  **Proof**: per-band trace first; `regress`; the three regress scenes and the forest at 2x and 4x `cmp`-ed serial
  against workers; the shore unit check (F-01); `unit` ring seams 0.0000 and `capsule` 0.00%; the terrain sections in
  four alternating A/B pairs. Effort L (a thorough rework of a 230-line function).
* **The water corners once per vertex** (`surface_view.cpp:2442-2473`, `1894-2062`). A corner with its own water level
  recomputes the same `toView`, `length`, `normalize`, `pow 28`, two `sin`, `pow 4`, `exp` and `rainWall` in the four
  cells that share it (and today in every band): the libm calls under `drawTerrainLOD` are 28-45% of its samples in
  every window, ~1.0 ms per frame serial on the bench's sea views. **Change**: factor lines `2449-2470` verbatim into one
  `RVert waterCorner(x, z, wl, hy, t)`; `vertexOf` stores the result in a `VtxCache` field (+48 bytes per vertex, ~3 MB
  for the near ring) when `vc.wy > -1e8`, using the stored floats `(double)vc.wy`, `(double)vc.hy` (never the unrounded
  values) and `t == lastT`; the cell loop takes the field for corners with their own level and calls `waterCorner` only
  for mean-filled corners; the `sin`/`cos` of `windDir` stay adjacent statements so clang fuses them into the same
  `__sincos_stret`. **Gain** -0.8-0.9 ms on the sea rows standalone (every band repeats the maths today), -0.15-0.25 ms
  once the binning has culled the wet cells by rows; -0.75 serial. **Proof**: `regress` (`felisian_shore` draws water at
  1x), `cmp` of `bench_surface*.png`, `bench_drone.png` at 4x against the previous binary and serial against workers,
  `VESPERIS_SKIP=wq` to bound what remains.
* **The warm pass's serial third walk** (`surface_view.cpp:2248-2263`). After the parallel pass over cell rows, a serial
  loop over every row of every ring re-evaluates the culls and warms the top corners (~20,600 cell visits, ~0.3 ms with
  the pool idle). **Change**: one `parallelFor` over `2 x radius + 1` vertex rows: the thread owning rows `[vb, ve)` tests
  cell `(cx, vz)` and cell `(cx, vz - 1)` with the existing predicate and warms `(cx, vz)`, `(cx + 1, vz)` when either
  passes: the same set of vertices, each written by exactly one thread; in `vertexOf` take `cache.at` only after the
  stamp test fails. **Gain** warm 0.8 → ~0.5 ms at every scale (6% of a 1x frame). **Proof**: the `warm` trace lines,
  `regress`, ring seams, `capsule`, the harness stamp assert.
* **Adaptive row chunks for the terrain** (after the binning). Standing on the ground, lod1 projects into row 401 alone
  at 4x and lod0 into rows 402-413, while the near ring spreads over hundreds of rows: one band holds most of the drawn
  cells. **Change**: while binning, a per-row cost histogram (difference array, `1 + 4*wet` per row of a cell's range),
  `K = 4 x threads` boundaries by cumulative cost (every chunk at least one row), the chunks pulled from an atomic
  counter (serial when threads == 1). **Gain** ~0.3-0.8 ms at 4x on the sea (a single lod1 row's water set-ups,
  ~1.2 ms on a performance core, cannot be split by rows: the per-vertex corners above are what lower that floor); up to
  ~1 ms on hilly worlds and the forest where the horizon band's cells span rows. **Hazards**: a boundary bug draws a row
  twice or never (the serial comparison at 4x catches it); ~60-80 lines; a second scheduling primitive, so build it only
  with the per-band trace showing the skew, and only after F-06: `WorkerPool::run(count)` starts `count - 1` workers
  today, so 32 chunks need the dealing primitive first. Effort M.
* **Done 2026-10-14** (`docs/decisions/log/4-1.3.1.md` F-04), the first three items, the pixels the previous binary's. The water corners once per
  vertex (`waterCorner`, `VtxCache::wv`), the warm's vertices by rows of vertices, and the cells binned once on the pool
  (`binTerrainLOD`): each cell listed, in the walk's order, with the rows its pieces reach, each piece projected and clipped at the near
  plane as the rasteriser does it, so the bin knows rows and not bands (no shared band layout needed). Bounding a cell by all its
  points at once was too loose (a deep sea's bed thousands of rows under its sheet); by piece the bench's sea view lists 3,194 cells and
  none goes to every band. The terrain section 5.2 → 1.2-1.6 ms at 4x and 4.2 → 0.7-0.8 at 2x (the estimates 2.3-3.7 and ~2); surface
  4x 8.93 → 4.85 ms, 2x 6.56 → 3.09, 1x 5.28 → 1.83; the 4-to-8-thread slowdown gone (1x: 3.80 at four threads and 5.28 at eight before,
  1.97 and 1.77 now); buggy 4x 11.88 → 7.76, drone 2x 7.35 → 3.38, forest 4x 15.35 → 13.21, the descent's first frame 22.95 → 19.85.
  The warm stays at 0.8 ms (the water corners moved into it). TSan found a race the vertex rows made frequent: the fill's disc reached
  two cells past the radius and a corner reads up to 2.83 (four and a half on a lava world's near rings): the disc reaches three now.
  Left: the adaptive row chunks (after F-06; the horizon band now holds nearly every cell: 3,193 of 3,194 at 4x, 1.28 ms).

### F-05. The passes one thread still draws alone (S-M, identical)

The audit found six full-frame or large serial passes beside the parallel ones. Each takes the pattern the tree already
uses (`flora.cpp:232-241`, `forms_draw.cpp:322-331`: `parallelFor(FBH, 32, ...)` with `setRasterBand`, every band
walking the same primitives in program order), or the row-parallel pattern of the aurora grid.

* **The cabin** (`hud.cpp:60-64`, `cabin.cpp:130-298`, `drone_draw.cpp:160-268`). After the parallel space scene,
  `drawCabin` and `drawDroneRack` issue ~100-180 `rasterPolygon` calls (floor, the twelve-piece ceiling ring, four walls
  of seven quads, the cage, the capsule dome, the consoles, the rack with three drone meshes, the desk) on the main
  thread with no bands: 160 of 1,516 main-thread samples in the serial space-4x window, 4.3 ms per frame, 31% of the
  13.6 ms parallel frame, the Amdahl term of every space row (the telescope row, cabin off, scales 2.6x). The solid hull
  runs the grain loop (`solid.grain = &cabinGrain`), the depolarised hull the darken/sheen path. **Change**: hoist the
  per-frame scalars (`shardsPending()`, which builds a `std::map` and a `std::set` per call, `droneBayReady()`, the three
  rack places and poses, `flick/breathe/sweep/glow/shine`); move the body from `RasterParams solid` through the desk and
  screens into `drawCabinBand(yb, ye)` run by `parallelFor(FBH, 32, ...)` with `setRasterBand`; `rs.bank` of the screen
  lambda becomes a per-thread local; the roof branch's early return becomes a flag; the bulb and fuel orb
  (`rasterPoint3` + `glowDisc`, not band-clipped, `skipOccluded`) stay serial after the bands, as today they are last.
  **Gain** -2.8 ms on space 4x (13.6 → ~10.8; residue 1.1-1.5 ms of static-band imbalance: the hull's pixels sit in the
  top and bottom bands), -1.5 at 3x, -0.6 at 2x, -0.5 on belt, vimana and probe 2x; nothing serially. **Hazards**: the
  regress never saw the cabin (`space_S00` calls `sr.render` directly), so the F-01 unit check is mandatory, and a 1x
  cabin frame could join the regress hashes (blessed once, reason recorded); forgetting the glow ordering or the
  per-thread bank moves pixels only in the cabin; B-411 showed the pixel path is codegen-sensitive, so judge by
  alternating bench pairs. Effort M.
* **The map and menu screens' dim and blit** (`font.cpp:136-147`, `landing_map.cpp:402-483`, `hud.cpp:458-466`, 14
  full-frame `blendRectRGB` sites). The landing map at 4x renders the 13.6 ms space scene, blends the whole frame
  serially, then re-lights 524k pixels (an integer division by the texel width, a dot, a smoothstep, a shadow divide,
  three products and casts per pixel, ~5-7 ns each: 2.5-3.5 ms) although sixteen pixels share a texel; the sector map
  blits 512k pixels with two double divisions each (~1-1.5 ms). The landing map at 4x is ~17-18 ms, over budget.
  **Change**: `blendRectRGB` over physical rows under `parallelFor(rows, 64, ...)` (the thousands of 1x1 calls stay
  serial under the threshold); the landing map per texel row: a thread-local row of texel colours computed once with
  the identical expressions, then `row[px] = rowCol[px * W / PW]` (the whole-world view skips every second texel at 1x,
  so a block fill would move pixels; the row cache does not); the sector map's `ix[x]`, `iy[y]` tables with the very
  same expressions and a gather loop over parallel rows. **Gain** landing map at 4x -2.5-3.5 ms (back under 16.7),
  sector map -1.5-2, menus, help, decoder, case, bay, sheets -0.5-0.75 each (the blend alone is ~0.9 ms per full
  frame by its own samples). **Rule to write down** in `font.cpp`: a blend
  that may dispatch never runs inside a pool job (the pool is not reentrant; today every RGB pass runs after the mush on
  the main thread). **Proof**: these screens are outside the regress; `cmp` of their frames at 1x/2x/4x against the
  previous binary and serial against workers; the F-01 bench rows for the timing.
* **`glowDisc`'s serial disc** (`framebuffer.cpp:85-110`, `surface_view.cpp:3067-3083`). `drawVeil` adds the veiling
  glare with one disc of radius `0.42 x FBW` (538 px at 4x, ~0.91 M pixels when the sun is near the centre of the view)
  walked serially per pixel (a sqrt, a divide, a read-modify-write), in every daytime frame with the sun within ~45° of
  the view; when `veil < 0.1` every `add` is 0 and the disc is still walked. No bench row faces the sun. **Change**:
  `if (maxAdd <= 0) return;` after the radius test, and `parallelFor(y1 - y0 + 1, 32, ...)` round the row loop with the
  per-pixel body unchanged (small discs stay serial under the threshold); all ten call sites run on the main thread
  outside parallel regions, verified. **Gain** 0.8-2.5 ms per daytime frame facing the sun at 4x (~1 ms walking,
  ~2.5 looking at it), 0.3-0.6 at 2x; the early-out saves the whole walk in dim-veil frames. **Proof**: a sun-facing
  scene at 4x `cmp`-ed serial against workers; the F-01 row.
* **The nose camera and the radar camera** (`buggy.cpp:246-272`, `radar.cpp:453-521`, `hud.cpp:311-313, 62`). In the
  seat view and with the radar camera on, one serial loop rewrites every pixel after the mush: a dependent xorshift32
  step per pixel in raster order, a modulo, two LUT reads, three multiplies, a clamp. Bounded by the bench: the feed is
  ~1.2 ms of buggy 4x's +3.06 ms over surface 4x (the rest is the hull, tracks and puffs); the radar feed is heavier and
  unmeasured (~2-3 ms serial at 4x on the 13.6 ms space frame: over budget with the radar on). **Change**: xorshift32
  (13, 17, 5) is linear over GF(2), so one step is a 32x32 bit matrix `T` and `T^W` (W = FBW) is computed once per width
  (eleven squarings of 32 column words); `rowSeed[y + 1] = T^W x rowSeed[y]` serially (800 x 32 XORs); the row loop runs
  in `parallelFor(FBH, 32, ...)` with `rs = rowSeed[yb]` at each band's start and the per-pixel body byte for byte; the
  radar's `static rowGlow` becomes per-band scratch (it would race). The helper lives in `core/rng.h` (~20 lines) with a
  unit check that `T^W` applied `FBH - 1` times equals `FBW x (FBH - 1)` single steps for every scale. **Gain** -1.0-1.3
  ms per frame at 4x in the seat view (buggy 4x 11.9 → ~10.7), -0.25-0.3 at 2x; the radar camera ~-2 ms at 4x
  (estimate). **Hazards**: a wrong shift direction or column convention gives a different speckle that no hash catches
  today: the unit check and the F-01 feed frames are mandatory; re-seeding per row from a hash would be five lines but a
  different speckle (hash-moving), so the exact way is kept. Effort M for ~1 ms in a non-default mode: last in F-05.
* **The sky's cloud and zodiacal grids** (`surface_view.cpp:2516-2576, 2829-2839`). The cloud grid is 81 x 51 = 4,131
  cells at every scale (`CELL = 4 x FB_SCALE`), ~2,000-2,400 above the horizon each evaluating three `fbm2` (ten
  `gnoise2`) plus the front's reads, serially on the main thread before the parallel sky pass: 0.55-0.7 ms per cloudy
  frame at any scale (~10% of a 1x frame; the reason the forest's sky scales 2.9x against the clear rows' 3.9x); the
  zodiacal grid runs `zodiacalLight` (acos, asin, two pow, three exp) for all cells on dusty nights, serially. The aurora
  grid thirty lines below already runs under `parallelFor(GY, 4, ...)`. **Change**: the same two lines round each loop;
  the bodies read only constants and pure functions and write their own cells. **Gain** ~0.45 ms per cloudy frame at
  every scale (the work sits in the upper rows, so ~4x of 8), ~0.4 ms serial on dusty nights unmeasured. **Proof**:
  `regress` (the three surface scenes have clouds), the forest frame `cmp`-ed serial against workers, a front scene too.
* **The floor and the reflections** (`surface_view.cpp:2165-2193, 356-437, 3639-3647`). `drawFloor` (144 far polygons)
  runs serially before the rings and `drawReflections` (the mirrored lod0/lod1 cells, a per-frame sort, `BLEND_REFLECT`)
  serially after them: in the forest the reflections are 1.0 ms per frame (the printed `objects 1.0`), the floor 0.19.
  `drawObjects` (the rocks) is 0.03 ms: leave it. **Change**: call `drawFloor` at the top of the ring band lambda; split
  `drawReflections` into a serial `gatherReflections()` (today's cell loop, sort and B-322 clip, pushing each polygon's
  vertices and projected rows into a reused member vector) and a `drawReflectionBand(fb, yb, ye)` called in the same
  lambda after the ring loop. The gather must stay serial: during the descent the lod0 cells it reads are not warmed
  and `TerrainCache::at` writes on a miss. **Gain** forest 4x ~0.55 ms (reflections 1.0 → ~0.45, the floor 0.19 →
  ~0.05); shore scenes alike; the plain surface rows under 0.05. **Proof**: `regress` (`felisian_shore` has
  reflections at 1x), `scene felisian_shore scale=4` and `bench forest` `cmp`-ed; `VESPERIS_SKIP=refl` still works.
* **The sun's glow pass when the cone misses the frame** (`space_view.cpp:335-349, 355-432`). The box round the sun is
  computed only when `centreFront && theta + outerA < 85°`; otherwise the glow pass visits every pixel of the frame (a
  ray, a sqrt, a dot, a divide, a compare) and writes nothing when the sun is beside or behind the camera: 1.1-1.2 ms
  serial per frame in the surface and space rows, where the sun is out of the window. **Change**: a five-line
  `coneMissesFrame(pj, theta, halfAngle)`: the corner angle from the farthest pixel centre (`max(|0.5 - cx|, |FBW - 0.5
  - cx|)` and the y twin over `f`); skip the pass when `theta - cornerA > halfAngle + 1e-5` (the spherical triangle
  inequality: no pixel can be inside the cone); the same guard round the protostar strip and, after the jet's glows,
  round the black hole's frame copy, bend grid and lens pass. The rays and the point sun stay. **Gain** -0.25-0.3 ms on
  space 4x and surface 4x, -0.07 at 2x and on the telescope; the forest 0 (its sun at 45-75° off-axis keeps the loose
  square box: a per-row extent of the cone's conic section is a separate follow-up). **Proof**: `regress` (`space_S00`
  and the surface scenes have the sun in view), a unit case at `theta = cornerA + outerA ± 1e-4` hashed against the old
  binary, `cmp` of `bench_surface*.png`.
* **The sun's loop and the stack** (found measuring F-04, KI-409). With one stack alignment the per-pixel lambda of `drawSun`
  took five times its samples: a one-thread bench launched as a zsh background job with only `VESPERIS_THREADS=1` added drew
  the sun-facing row at 4x in 60-63 ms against 26 (F-04's binary, six runs; any other environment 25.7-26.2). The lambda reads
  every capture through its closure from `drawSun`'s frame while it stores pixels. **Change**: copy the captures the loop reads
  into the band body's locals (identical arithmetic). **Proof**: that launch's sunward row against the others; the regress's
  `space_S00` and the sun-facing frames `cmp`-ed.

* **Done 2026-10-14** (`docs/decisions/log/4-1.3.1.md` F-05), every item, the pixels the previous binary's. The cabin in bands
  (`drawCabinBand`, the frame's values once in a `CabinFrame`, the drones' models made once, the bulb's and orb's glows after
  the bands): space 4x 14.09 → 10.62 ms (the estimate ~10.8), 2x 4.14 → 3.20, the belt 2x 2.02 → 1.17, Vimana 2.21 → 1.47, the
  decoder 13.25 → 7.50. The screens: `blendRectRGB` over rows from 100, the landing map's blit and zoom by rows with a texel's
  colour made once (7.20 → 4.47 and 7.81 → 4.97: the estimate 2.5-3.5), the sector map's index tables (9.87 → 8.34).
  `glowDisc`'s early-out and rows; the feeds by rows with xorshift32's row jump over GF(2) (`xorshiftRowStarts`, the unit's
  check): buggy 4x 7.80 → 5.92 (the estimate -1.0-1.3), the radar camera 6.69 → 5.03; the sky's cloud and zodiacal grids on the
  pool; the floor and the reflections inside the terrain's bands (`gatherReflections`, `drawReflectionBand`, `polygonRows`
  shared with the bin): forest 4x 13.32 → 11.80 (the estimate -0.55); the sun's cone test (`coneMissesFrame`, also round the
  protostar's strip and the black hole's lens; `suncone` sweeps the edge, both binaries the same): sunaway 7.98 → 6.17, the
  sprint 7.67 → 5.71, sunward 10.68 → 9.04. KI-409 closed by the glow loop's locals (in its launch sunward 61.7-62.1 → 25.5-25.8
  ms). Proved by the new `screens` (96 of the game's own frames at 1x-4x, the same between runs) and the usual set; the flow's
  wait for the scanner now holds the instant (a faster binary's clock had run ahead from there). Left: the cabin's hull fills
  the top and bottom bands (F-06's dealing); a 1x cabin frame for the regress (F-11).

### F-06. The pool deals its rows (M, identical)

`parallelFor` (`parallel.h:84-95`) cuts `[0, n)` into `min(parallelThreads(), n / minPerThread)` equal contiguous bands
and `WorkerPool::run` gives band `t` to thread `t` for good. On four performance and six efficiency cores (eight threads
cap), four bands run at ~0.55-0.6x speed and the frame waits for them; the rows are uneven too (a globe's two middle
bands hold 15.8% of the disc each against 6.7% for the outer ones; the sky skips the ground rows; forests and cities fill
two or three bands of eight). The space-4x curve 40.6/22.6/16.0/15.3/14.0 ms at 1/2/4/6/8 threads fits a serial ~5 ms (the
cabin) plus a parallel ~36 ms at an effective 2.05/3.3/3.6/4.1 threads: the per-pixel passes reach about four of eight.
`probe_view.cpp:90-95` already deals its columns from an atomic counter "because the efficiency cores would hold up an
even split".

* **Deal chunks from an atomic counter** in `parallelFor`: the caller included, each participant loops
  `for (int b; (b = next.fetch_add(chunk)) < n;) fn(b, min(n, b + chunk))` with `chunk = max(minPerThread / 2, n /
  (threads * 4))`; the `threads <= 1` path (`VESPERIS_THREADS=1`) untouched; delete the probe's `dealt()`. Every pure
  per-row pass gains unchanged: the mush, `toRGB`, the sky, the galaxy band, the aurora, the sun, the globe, the black
  hole's grid and lens, the warm vertex rows, the probe's panorama, the landing map, the charts; the band-stepped passes
  (sky, band, aurora at `step`) land every chunk edge on a step multiple, verified. **The band-raster passes are
  different**: the terrain, the flora and the forms keep today's band count `min(8, FBH / 32)` pinned by a named
  constant, independent of the thread count, until their own set-up fixes (F-04's bins, a flora pre-pass): with their
  replicated per-band set-up finer chunks cost more, and the flora is layout-dependent until F-01's check says
  otherwise. **The plate** (`plateFill`, `space_view.cpp:692`): the 8,000-sample budget per frame is a feature (the
  sharpening's pace is machine-independent in frames) and stays; its blocks are dealt in chunks of 64 (disjoint tiles,
  `taken` an atomic sum, the same blocks land in the same frames). **Gain** space 4x -2 to -3 ms (the globe 6.7 → ~4.5,
  the band -0.3, the mush -0.25, the sun -0.2), telescope 2x -1.5 plus ~0.6 from the plate, surface 4x -0.4-0.7 (the
  sky), forest 4x ~-0.5; terrain, flora and forms 0 until their own items. **Pixels**: only which thread computes a row
  changes (the X-02 precedent); every per-row lambda was read for cross-row state: none.
* **A bounded spin before parking** (`parallel.h:31-43, 57-74`): each dispatch pays two kernel wake-ups on the critical
  path (notify_all to the workers, `done.notify_one` back), 10-30 µs each, ~15 dispatches per surface frame and ~8 in
  space. **Change**, only if F-01's microbench reads over ~25 µs per dispatch: `generation`, `pending` and the job pointer
  atomic; a worker spins ~20-50 µs on `generation` before `cv.wait`, the main thread on `pending == 0` before
  `done.wait`; `pending.fetch_sub` without the mutex; notify only parked workers with `idx < curCount`. Publish with
  release, read with acquire; a `-fsanitize=thread` scratch build of `unit` and a stress loop at 2 and 8 threads.
  **Gain** -0.3-0.6 ms per surface frame, -0.15-0.3 in space, 0 serially. **Hazard**: spinning workers compete with
  the drainage samplers and the ahead worker on a ten-core machine; on a four-core box it can slow the frame; keep the
  bound and A/B it.
* **A `thread_local` reentrancy guard**: a nested `parallelFor` runs `fn(0, n)` instead of deadlocking the pool, now
  that `glowDisc` and `blendRectRGB` (F-05) can dispatch from many places.
* **The picture becomes thread-count independent** (B-423's pinned layout plus the dealing): a unit check that
  `VESPERIS_THREADS=2, 4, 6, 8` give identical PNGs on six scenes at 4x; a debug assert that only the main thread
  dispatches; a `docs/decisions/log/4-1.3.1.md` row narrowing M7-04's "fixed thread bands" sentence (the hashes stay deterministic
  because every pass is row-pure or band-pinned, not because the bands are fixed to threads).
* **The cap of eight threads** (`parallel.h:21`) on ten cores: a separate, measured step after the dealing, because
  B-422 counts cores for the drainage workers ("every core but two") and the ahead worker; expected +0.5-1 ms on space 4x
  when the efficiency cores are free, possibly a loss on the surface rows while a prefetch runs.

* **Done 2026-10-14** (`docs/decisions/log/4-1.3.1.md` F-06), the pixels the previous binary's. The dealing as planned (chunks of a quarter of a
  thread's share from an atomic counter, the caller included; completion by work: a worker that wakes after the body closed never
  enters it), the workers woken by one `notify_all` (14 us of the caller's after a rest, F-05's pool 43-45; 1.3 back to back against
  34), the probe's `dealt` and the plate's blocks (64 a chunk) on it, `parallelBands` dealt, the guard (`pool::inside`, `try_lock`,
  `pool::contended`). Measured and not built: the bounded spin (50 us, then in the M4's low-power `wfe` wait: the surface at 1x-3x
  0.15 ms faster, every space frame and 4x 0.1-0.4 slower), the terrain's chunks by cost (the plan's per-row model: surface 4x 4.31 →
  5.09 ms, forest 4x 10.74 → 12.21; the equal chunks keep the horizon's one of thirty-odd, 1.43 ms of the forest's 1.8 ms section),
  per-thread stretches (cache affinity: no better), eight pinned bands for the terrain, the cabin and the forms (slower: forest 4x
  10.61 against 11.41), the cap of ten (space 4x -0.53 ms, the telescope 4x -0.94, a recorded frame 7.7 → 14.5: kept at eight). The
  forms take chunks of eight rows or a sixty-fourth of the frame (city 4x 7.16 → 6.83). Space 4x 10.65 → 9.31 ms (the estimate -2 to
  -3), 2x 3.19 → 2.91, 1x 1.46 → 1.32; the telescope 4x 24.44 → 23.05 (2x within the spread: the estimate -1.5 and the plate's 0.6 did
  not come); surface 4x 4.81 → 4.18 (the estimate -0.4-0.7), 2x 3.12 → 2.46; forest 4x 11.89 → 10.60 (the estimate -0.5); city 4x 8.93
  → 6.69; sunaway 6.24 → 4.96; the decoder 7.32 → 6.12. New: the unit's six scenes at 4x the same with one, two, four, six and eight
  threads, the `pool` mode. Left: the horizon's chunk (cells a row high, a cell's set-up the floor), the flora's pinned bands (its
  per-tree set-up), the bench's buggy 2x row (its first frames beside the relocation's drainage work) and the recorder's writer beside
  a dealt pool.

### F-07. Exact trims (S each, identical; land in three or four commits with A/B pairs)

Each is small and provable; together they are worth about as much as one of the structural items. Order within the
milestone by measured gain, and judge each by four alternating `bench check` pairs against the saved previous binary
(B-411's lesson: a branch added to a hot pixel loop once cost 3-10 ms).

* **`wrap2pi`'s one-turn fast path** (`types.h:123-127`). Every map read of the globe (`albedoAt`, `materialAt`, four
  `heightAt`, `cloudAt`) calls `std::fmod` on a longitude that already lies in `[0, TAU]`: 133 of 1,516 serial samples
  in the space-4x window. **Change**: `if (a >= 0 && a < TAU) return a; if (a >= TAU && a < 2*TAU) return a - TAU; if (a <
  0 && a > -TAU) return a + TAU;` then `fmod`. Identical: `fmod` is exact; for `[TAU, 2TAU)` the subtraction is exact by
  Sterbenz; `(-TAU, 0)` returns `a` then adds TAU in both versions; `-0.0`, `±TAU`, `2TAU`, NaN and inf take the old path.
  **Gain** space 4x -0.45 ms (the cloud read's `fmod` keeps its slow path: `cloudDrift` is tens of radians from the
  clock's 3.6e6 s start, which F-10 fixes), probe 2x -0.1 (`panoAt`), nothing on the plate. **Proof**: a unit sweep of
  `a` in `[-3TAU, 3TAU]` plus the special values against the old formula by `memcmp` (bits, not values: the `-0.0`
  case); `regress` (`space_S00` and the ten maps exercise it).
* **The map accessors and `TerrainCache::at` inline across translation units** (`planetmap.cpp:2441-2468`,
  `site.cpp:23-30`). The globe's seven lookups per pixel are one-line functions in another file, so each is a call that
  reloads the data pointer and computes its own `wrap2pi`; the thin-LTO build (which inlines them at link time and kept
  22/22 hashes) measured the ceiling: 0.8-1.7 ms on space 4x. **Change**: `texelIndex`, `albedoAt`, `cloudAt`,
  `heightAt`, `materialAt` and the `bilinear` template into `planetmap.h` (a private static member template, not an
  anonymous namespace in a header); `TerrainCache::at` inline in `site.h` with the cold branch out of line as
  `fill(TerrainVertex&, int, int)` so the header never pulls `sampleSurface`. GVN then shares `wrap2pi(lon + PI)` across
  three of the seven lookups (fmod is pure under Darwin's no-math-errno default). **Gain** space 4x -0.5-1.0 ms, surface
  4x -0.1-0.15, every zip and the dev bench (LTO would not reach them). **Proof**: `regress` (`space_S00` runs the
  accessors at 1x), `cmp` at 4x.
* **Exact trims in the globe's per-pixel path** (`space_view.cpp:976-1091`). The bench body has two moons, so for every
  lit pixel the shadow loop runs two occluders (a subtraction, two dots, a sqrt each) though neither can shadow the globe
  in most frames; `giantJetTurn` is an out-of-line call returning 0 for every type but a gas giant; the plate's cloud
  rotation recomputes a sincos per pixel for a per-plate constant angle; the limb's `pow(1 - viewCos, 3)` runs on the
  night side where it is multiplied by 0; `exposureStop`'s fractional pow runs for giants and opaque decks whose value
  is overwritten. **Change**: per frame per occluder `rel_c`, `along_c`, `dp_c`, and drop it when `along_c <= 0` or `dp_c
  - R > o.r x (1 + 1e-6)` (the bound: `dp >= dp_c - R`, `along < along_c` for lit points, so the loop would leave `shadow
  = 0`); `giantJetTurn` declared inline in `probe.h` (no type test at the call site); `cs/sn` of `cloudDrift -
  cloudDrift0` per plate per frame for `jet == 0`; `if (lit + 0.2 > 0)` round the limb term; `exposureStop` only when
  not `(atmosphere && (opaque deck || gas giant))`. The ring-shadow skip as the finder stated it is wrong (`tr > 0` on
  half the globe for any star direction): use `ringR0 x |dot(starDirW, spinAxis)| > R` or leave it. **Gain** space 4x
  -0.5-0.8 ms (the moons' loop 0.3-0.6), telescope 2x -0.5-0.9, a parked gas giant another ~0.25. **Proof**: `regress`
  (`space_S00`), the flow's telescope, moon, giant and ringed frames `cmp`-ed; an eclipse frame the regress lacks
  looked at.
* **Replay the galactic backdrop while the camera is still** (`space_view.cpp:1254-1291`, `starfield.cpp:124-134`). Every
  parked frame recomputes, per 2x2 block, a normalize, `atan2`, `asin`, `cbrt` and eight integer modulos whose inputs
  (`c.cam`, `proj`, the band map, no patches) did not change: 5.7 ms serial, ~1.2 ms with workers. **Change**: (a)
  `sampleGalaxyBand` takes `BAND_MAP_W/H` as constants and wraps with `& (W - 1)`, once per coordinate (identical for a
  power of two; the four callers drop the size arguments): ~0.1-0.2 ms on every frame, the surface sky's calls too; (b)
  in `render`, when `!c.vimana && nebN == 0`, a key `{c.cam, proj.f, cx, cy, FBW, FBH, bandPos, bandValid}`: if it equals
  the stored layer's, `memcpy` the layer instead of `clear + pass`; else clear, pass, and store `fb.idx` when the key
  equals the previous frame's. `bandPos` is the right key for the band map (both writers set it: the parked build and
  the flight's handover; `parkedBandBuilds` misses the handover). The telescope never hits (the stabiliser and the
  tracking move the camera every frame); the cabin's look is in `c.cam` via `viewBasis`. **Gain** space 4x -1.1 ms
  parked and still (-0.8 after F-06), space 2x -0.3; nothing while turning, in flight or under nebula patches. **Hazard**:
  a later input to the pass not added to the key gives a one-frame-old sky that no regress catches; the key's
  completeness is the whole contract (note it in `02-rendering.md`); +2 MB. The one item in F-07 that adds state: last
  in the milestone, optional.
* **The mush: column sums, one dispatch per pass, an exact reciprocal, a buffer swap** (`framebuffer.cpp:25-83`). The
  game's default fine kernel is `k = max(2, S - 1)` = 3 at 4x and 2 at 2x/3x (the measurer's "k = 5 at 4x" holds only
  for `bench forest` and harness modes without a `Game`). The general path copies the frame per pass, runs a horizontal
  running sum into a 4 MB buffer, then per pixel re-adds k rows and divides by a runtime divisor; the `k == 2` path is
  fully serial. 125 of 1,516 serial samples (3.35 ms for two passes at 4x), ~0.75 ms with workers. **Change**: one
  `parallelFor` per pass over bands of output rows, each band sliding a column sum `col[W]` down one row at a time, a
  k-wide window over it, `rows` hoisted per row, the interior (`cols == k`) split from the `k - 1` tail columns, and the
  division as the exact reciprocal `q = (a x m) >> 32`, `m = floor(2^32 / d) + 1` (exact while `a x d < 2^32`: here `a
  <= 2047 x 36`); the `k == 2` loop in the same bands (rows independent), copying rows `H - 2`, `H - 1`; a `back`
  member and `idx.swap(back)` instead of the memcpy; the bank packed into the sum's upper half so no copy is read.
  Drop the `toRGB` LUT cache (<=0.05 ms for an invalidation rule). **Gain** -0.35 ms at 4x on every row (-0.5 with three
  passes), -0.2-0.6 at 3x and -0.1-0.3 at 2x from banding the `k == 2` path. **Proof**: the regress runs only the `k ==
  2` path (1x), so frames with `g_mushKernel` 2..6 (the sheet code sets it, `main_headless.cpp:3461`) `cmp`-ed against
  the previous binary at 2x/3x/4x, serial against workers; a brute-force unit test of the reciprocal over the whole
  range; a `static_assert` on the packing.
* **Rasteriser trims** (`raster.cpp:9-39, 44-53, 81-166, 187-224`). `emitPixel` is a separate symbol per pixel (14-18% of
  the raster time) that re-tests a depth the textured loop already tested; `rasterProjected` divides `1.0 / iz` per pixel
  even where a polygon's vertices share one view z (leaf clusters, lily pads, blob shadows); it sorts and divides before
  its band early-outs; `rasterPolygon` copies through the near-plane clip even when every vertex is in front and never
  tests the band's rows, so every band pays every fan triangle's set-up. **Step 1** (identical, trivial): `z = 1.0 / iz`
  once per span when `diz == 0.0` exactly; `lerpZ1/2` below the early-outs; a band bbox reject in `rasterPolygon` and
  `rasterTriangle`; SVs by reference; the all-in-front fast path projecting `min(n, 15)` vertices (the clip loop breaks
  at 15 today, so the cage ring's 16th vertex is dropped: a fast path must keep that). **Step 2** (judge by A/B): the
  opaque `BLEND_REPLACE` body inline taking the biased `iz`, `DARKEN/REFLECT/ADD/MAX` and quantize out of line. **Gain**
  forest 4x ~-0.3 ms (0.2-0.5: it hinges on the divider being on the critical path), city 2x -0.1-0.2, the cabin's share
  until F-05. **Proof**: `regress` (`felisian_shore` drives both loops), the forms check, `cmp` of the forest, city and
  the cage ring at 4x.
* **The sky's exact skips** (`surface_view.cpp:2693-2773`). `pow(1 - clampd(el, 0, 1), 2.2)` is called with 1.0 for the
  ground half of the frame; the two glow pows run when `sunUp == 0` (night) or the pixel faces away (`cs == 0`), where
  their term is exactly +0.0; a ringed world's arc calls pow on one of 256 profile values per pixel. **Change**: `hz =
  el <= 0 ? 1.0 : pow(1 - min(el, 1.0), 2.2)`; `if (sunUp > 0 && cs > 0)` round each sun's glow, `cosSun > 0` for the
  airless pow 300 and the comet's; a 256-entry `ringPow` table per frame. Skip the integer-division hoists (clang already
  hoists per row). **Gain** ~0.05-0.1 ms on the bench rows (the profile shows `cs > 0` for nearly every bench sample),
  ~0.8 ms at 4x at night or facing away from the sun, ringed worlds extra. **Proof**: `regress` covers all three paths
  (`molten_night` has `sunUp == 0`, `cratered_noon` is airless, `felisian_shore` is day).
* **The halo and pillar pre-tests** (`surface_view.cpp:2718-2724`). On cold clear worlds every sky sample pays an `acos`
  to find whether it lies within 1.4° of the 22° ring (1.5% do), and with a low sun every sample within 32° of it pays
  `atan2 + asin` for a 5° x 13° column. **Change**: per frame `cosHaloLo/Hi = cos((22 ± 1.401)°)`, `tanAz = tan(2.501°)`,
  `elLo/elHi = sin(sunAltA ∓ 2.001° / + 11.001°)`; test the interval before the call, keep the original condition after
  it (monotone functions, a 0.001° margin: nothing the branch would add is skipped). **Gain** 0 on the bench; ~0.6 ms
  per frame at 4x on icy, europan, bombarded and winter worlds, up to ~1.5 more facing a low sun. **Proof**: `scene
  icy_noon` at 4x and a cold dawn `spot` `cmp`-ed.
* **The telescope plate's exact per-sample work** (`space_view.cpp:703-712, 1072-1076`). The plan cores' loop runs
  `std::acos` per core before the reach test; the plate's cloud rotation recomputes a sincos per pixel. **Change**:
  `cosReach = cos(min(PI, (k.r + texelM) / Rm))` stored per core, `if (dot(u, k.u) < cosReach - 1e-9) continue;`; the
  sincos hoisted per plate per frame. The lazy lat/lon is left out (a flag over six use sites for a gain the bench never
  sees). **Gain** ~0.15 ms at 2x; up to 0.2 ms x cores in reach over a people's world.
* **The probe's panorama step table** (`probe_view.cpp:269-320`). Every march step of every column computes `pow(1 +
  STEP, k - rLin / minStep)`: ~60-130k non-integer pows per frame. **Change**: a per-call `std::vector<double> rTab` up to
  `kEnd(rMax) + 2` with the identical expression, read by index (not stored in the static `Pano`: `D.sh` changes it).
  **Gain** probe 2x -0.2-0.4 ms (the panorama section 1.7 → ~1.4). **Proof**: `bench_probe.png` and the flow's probe
  frames `cmp`-ed, `probe stability` sheets unchanged.
* **The noise's gradient switch and the Worley hashing** (`noise.cpp:9-32, 129-147`, `planetmap.cpp:121-141`). The
  sceptic found clang already shares the X and XY hash prefixes in `gnoise3`/`gnoise3d` (31 multiplies = 14 `mix64`), so
  the finder's factoring buys nothing there; what remains is `grad3`'s `switch (h & 15)`: eight indirect jumps per call
  on a random hash (up to ~8 mispredicts; `grad3v` is already branchless), and the chains recomputed in `worley3`'s and
  `visitFeatures`' 27-cell loops. **Change**: `grad3` as a 16-entry table `{a, b, sa, sb}` returning `sa x v[a] + sb x
  v[b]` with `sa, sb` in `{+1.0, -1.0}` (exact: multiplication by ±1 is exact, `x - y` is `x + (-y)`, addition commutes,
  an FMA contraction rounds once the same operands); prefix-factored hashing in the two triple loops through `hashX`,
  `hashXY`, `hashXYZ` in `rng.h` with `hash3i` as their composition. Keep it only if `bench noise` (F-01) shows the gain.
  **Gain** none on a steady 4x frame (the maps and caches are read); per event: telescope 2x -0.2-0.4 ms on the filling
  frames, the descent's first frame -3 to -8 ms, a tile's sampling -5 to -12 ms wall, a map -10 to -30 ms. **Proof**:
  `regress` (the ten maps and three profiles hash the planet function: 0 changed or it is a bug), the equality count of
  `bench noise`.
* **The synth's per-sample constants** (`audio.cpp:58, 81, 85, 164, 172`, `audio_medium.cpp:62-124`). Smoothing
  coefficients `1 - exp(-dt / tau)` and resonator radii `exp(-PI x BW x dt)` with constant inputs are recomputed per
  sample: nine per speech sample, six per phantom, five or six per medium sample. **Change**: a `Coef` struct filled once
  at the top of `render()` from `dt` and `st.medium` with the identical expressions; the per-sample `cos` and the
  data-dependent `exp` stay. **Gain** 0.1-1.0 ms per 2,048-sample block (one block in 5.6 frames) in the speech, murmur
  and medium states, the heaviest blocks. **Proof**: the harness's WAVs (`audio`, the radar, voice and probe modes)
  `cmp`-ed byte for byte before and after.
* **Flora riders** (`flora.cpp:613-917`): hoist the per-cell `section()` timers out of the cells loop (~2,700 clock reads
  per frame become three per band) and drop the cells' dead coverage marks (read by nothing after the trees). -0.04 ms,
  trivially identical. The flora's real rework is set aside (section 8).
* **`ifloor` for the x86-64 zips** (`noise.cpp:23`, `planetmap.cpp:2453`, `starfield.cpp:129`). On baseline x86-64
  (SSE2) every `std::floor` is a library call; arm64 and the Mac x86_64 zip (penryn, SSE4.1) inline it. `inline int
  ifloor(double v) { int i = (int)v; return i - (v < (double)i); }` is integer-exact for every in-range input and
  compiles to two instructions anywhere. 0 on this machine; plausibly 1-3 ms at space 4x on the Linux and Windows x86-64
  zips, unmeasurable from the Mac. Lowest ranked; a unit sweep against `std::floor`. Do not add `-march=x86-64-v2`
  (SIGILL on pre-2009 Intel and pre-2011 AMD CPUs).

* **Done 2026-10-14** (`docs/decisions/log/4-1.3.1.md` F-07), every item, the pixels, the sounds and the generation the previous binary's; landed in
  five steps with four A/B pairs each. The globe: `wrap2pi`'s turn, the lookups inline (`planetmap.h`, `TerrainCache::at` in `site.h`
  with its miss out of line, `giantJetTurn`), the occluders a frame (a body whose shadow cannot reach a lit point left out), the
  plate's cloud turn a plate, the limb on the lit side, the exposure where read, the cores' reach by cosine: space 4x 9.43 → 8.73 ms,
  the telescope 4x 25.38 → 23.05 (the ring-shadow skip left out, as the plan's verdict said). The mush (column sums, one dispatch, the
  reciprocal, `back` swapped, k = 2 on the pool; the toRGB cache not made) with the raster's step 1: surface 4x 4.10 → 3.57, forest 4x
  10.59 → 9.86. The sky's skips, the halo and pillar pre-tests, the probe's table (`marchSteps`), the noise (Perlin's selects:
  `gnoise3` 88.9 → 11.2 ns, kept by `bench noise`; the prefix hashing: `worley3` 230 → 187), the synth's `Coefs`, the flora riders:
  the telescope 4x 20.52 → 18.01, probe 2x 6.69 → 5.90, forest 4x 9.86 → 9.18. `ifloor` (the truncation's form on baseline x86-64
  only: on the M4 it cost `gnoise3` 1.5 ns). The backdrop kept while the camera is still, the sampler's size a constant: space 4x 8.21
  → 6.96, the landing map 3.33 → 2.33, the radar camera 3.68 → 2.45. The raster's step 2 (the opaque write inline, no second depth
  test): forest 4x 9.25 → 8.84, city 4x 5.76 → 5.41. In all: space 4x 9.43 → 6.86 (the estimate -1.5 to -3; the target 7 met with a
  still camera), the telescope 4x 23.06 → 18.37 and 2x 10.30 → 9.01 (the estimate -0.5-1), surface 4x 4.18 → 3.54 and forest 4x 10.37
  → 8.95 (the estimate -0.5-1; the forest's target 10 met), probe 2x 6.72 → 5.97 (the estimate -0.3); with one thread space 4x 40.37 →
  28.80 (the estimate's ~30 for the serial column after F-10). The night and cold worlds' gains are per scene (the bench has neither).
  New: the unit's three checks (the turn, the floors, the quotient); the flow waits for the starting system's maps and the probe's gas
  giant's (`testWaitMapOf`: a shot had a map or not by a binary's speed) and the unit's player check for its records (KI-410).

### F-08. The platform layer (S-M, decided by F-01's numbers)

* **The audio block off the main thread** (`platform_raylib.cpp:122-128`, `main_raylib.cpp:69, 86`). raylib's stream
  holds two 2,048-sample subbuffers; one frame in 5.6 synthesises a whole 93 ms block on the main thread before the
  present: ~0.25 ms quiet, ~1-3 ms with the ship's player or the radar, up to ~5 with 28 voices (from the per-sample
  operation counts; unmeasured). At space 4x (13.6 ms + ~1 present) that block is the difference between 16.7 and a 33
  ms frame every sixth frame while music plays. **Change**, only if F-01's trace shows the block over the vsync slack: a
  persistent helper thread; `platformAudioPump` launches `synth.render` on it and returns; `platformPresent` joins after
  `EndDrawing` (after the vsync wait) and calls `UpdateAudioStream`; `platformShutdown` joins before `UnloadAudioStream`;
  the pump call moves to just before the present so no game code runs between launch and join (the `AudioState`
  pointers are set and cleared only in `Game::frame`); Emscripten stays synchronous; one block per frame (two only after
  a 93 ms stall, inaudible). **Gain** the periodic missed vblank removed; 0.05-0.7 ms per frame on average (the critic's
  reading: this item is a dropped frame, not an average, and belongs with the cabin at the top of the steady-frame work
  once F-01's histogram shows it). **Rule**: this breaks a recorded contract (the synth on the main thread): a
  `docs/decisions/log/` row and a line in `01-architecture.md`. Also check `platform_raylib.cpp:70-73`: `FLAG_VSYNC_HINT` and
  `SetTargetFPS(60)` are set together, and raylib's wait on top of the vsync can alternate 16 and 33 ms frames when the
  CPU frame nears the budget; `SetTargetFPS(0)` with vsync on is a platform-layer experiment with no pixel change,
  decided by the histogram.
* **The texture upload**: measured first (F-01); a plain 4 MB copy is 0.07-0.15 ms on an M4 and only a driver swizzle
  could reach 0.5-1 ms. A pixel-unpack-buffer ring is an experiment for a scratch build only if the trace reads >= 0.5
  ms, with the WebGL fallback and the unbind rule in hand; not planned.
* **Done 2026-10-14** (`docs/decisions/log/4-1.3.1.md` F-08), the pacing; the helper thread and the unpack ring stay dropped (the synth's block 2.08
  ms at worst on a loaded Mac, the upload 0.42-0.77 ms: no frame waits for either). Measured in the window: with F-07's work (6-7.5
  ms) raylib's `SetTargetFPS(60)` ran every state at 17.2 ms a frame (58 a second: its sleep for 95% of the wait wakes late on macOS),
  2.0-3.4% of the frames on screen for 33 ms; the vsync alone (this item's experiment) mixed 8.3 and 16.7 ms frames on the 120 Hz
  panel (71 a second in the cabin) and was dropped. The platform paces instead (`platform_raylib.cpp`, `SetTargetFPS(0)` on the
  desktop): a frame is meant for the refresh a whole number of the monitor's refreshes after the last one's, its swap held until three
  quarters of a refresh before it (2 ms missed whole stretches under load), the schedule following the refreshes the swaps met:
  59.3-60 frames a second, 91-99.9% of the frames 16.7 ms after the previous, 0-0.5% at 33 ms, against 56.4-58.3, 57-94% and 2.0-3.4%
  (alternated, the same day). No pixel, no sound; the harness untouched. Unmeasured: other displays and the Windows and Linux builds
  (F-09's cross-builds compile it).

### F-09. The release flags (M, measured on the Mac; the containers decide the rest)

Thin LTO on the clang targets is the one flag that measured: three `-O2` runs (13.63, 14.01, 14.55 ms on space 4x) all
above both LTO runs (12.87, 12.83) while `-O3` alone sat inside the baseline band, and 22/22 hashes held (clang decides
`-ffp-contract=on` per source expression in the frontend, so link-time inlining cannot add a fusion). The telescope row's
difference is confounded by the plate's varying work; the surface rows are noise. **Step 1**: `-flto=thin` on compile and
link in the macOS and Windows (llvm-mingw) release lines, nothing else; commit `build-mac.sh`, `build-win.sh`,
`build-linux*.sh` into `packaging/` (they live in a `/tmp` scratchpad that gets wiped; the recipe says to rebuild them
from a transcript). **Step 2**, in the arm64 container: build `vesperis_test` with the old flags, `regress bless` to a
scratch baseline, rebuild with `-O2 -flto`, `regress` and `VESPERIS_THREADS=1 regress`, `bench check`, `cmp` the harness
PNGs; only then try `-O3` and `-fno-math-errno` the same way (GCC 9 on Ubuntu 20.04 lacks `-O2` inlining and keeps
`-fmath-errno`, so every `sqrt`/`pow` is a call; `-fno-math-errno` lets both compilers inline sqrt and fold `pow(x, c)`,
a possible one-ulp move on those zips: KI-014 already accepts per-platform hashes, but a Linux player's picture against
the previous zip is the question, so it is gated on the container's own old-versus-new regress). **Step 3**: Windows
`-fno-math-errno` only when a Windows `vesperis_test` run exists; until then the zip is documented as untimed. Keep the
Makefile at `-O2` (the edit loop; the bench must keep measuring code, not flags). Dropped: `-O3` on the Mac (noise),
PGO (six targets, a training run per build), `-fno-plt`/`-no-pie`/`-fno-stack-protector` (unmeasured micro-flags that
make the lines diverge), a newer Ubuntu base (glibc compatibility), `-march` (section 8). **Gain** space 4x -0.8-1.7 ms
on the Mac zips (the dev bench gains nothing); Linux and Windows plausible, unmeasured.

* **Done 2026-10-15** (`docs/decisions/log/4-1.3.1.md` F-09). The scripts are in `packaging/` (`toolchains.sh`, `build-mac.sh`, `build-win.sh`,
  `build-linux.sh` with its inner script and Dockerfile, `build-release.sh`). Thin LTO on the clang lines: every picture identical on
  the Mac (arm64, and x86_64 under Rosetta), 0-3% faster (space 4x 6.86 → 6.80 ms, the telescope 2x 8.86 → 8.61), one thread the same;
  the estimated -0.8-1.7 ms was measured before F-07's inlining took it. Linux: in the arm64 container `-O2 -flto` was 1.2% slower
  over the bench's steady rows, `-fno-math-errno` within the noise, `-O3` 4.9% faster (the three together 6.6%), every variant's
  pictures `-O2`'s; `-O3` taken, its pictures `-O2`'s in the amd64 container too (the regress both ways, 18 scenes, the screens;
  emulated, untimed). Windows takes the Mac's line, untimed (KI-411). The Windows builds' warning found B-424 (a telescope photo read
  no more of its plate: `1L << 40` in Windows' 32-bit `long`), and a scan for the narrowings only Windows has its second line (the
  loop's `std::min<long>`, which would have hung the photo); the Windows builds compile with `-Wshorten-64-to-32` now.

### F-10. The hash-moving batch, one bless (M, visually identical; the hashes move)

Three floating-point rewrites that cannot keep the last bits and so are batched, reviewed on the images and blessed
once, with the reason in `docs/decisions/log/` and the numbers in `08-testing-and-tools.md`. Done after the bit-identical
milestones so that every other change is proved against unchanged hashes.

* **The sky's glow pows as products** (`surface_view.cpp:2714-2715, 2693-2694, 2697`): `pow(cs, 6)` and `pow(cs, 50)`
  are 218 of the 508 sky samples in the serial 4x window (2.8 ms serial, 0.7 of the 1.7 ms sky with workers), the
  largest item of the sky section. `c2 = cs*cs; c6 = c2*c2*c2; c12 = c6*c6; c24 = c12*c12; c50 = c24*c24*c2`; the
  airless `c300 = (c50*c50*c50)*(c50*c50*c50)`; the comet's `c18 = c12*c6`; `pow(1 - el, 2.2)` stays. A few ulp from
  libm; the intensity is quantised by `toInten`, so a flipped pixel needs a value within ~1e-12 of a half step: the 22
  hashes are expected to hold but cannot be proved to. **Gain** surface 4x -0.65 ms (8.8 → ~8.15), 2x -0.6, forest 4x
  -0.7-0.9.
* **The globe's cloud drift and limb** (`space_view.cpp:961, 1077, 1090`): `cloudDrift = wrap2pi(c.t x TAU / (rotPeriod x
  3.7) + 1)` once per frame (so the cloud read's `fmod` takes F-07's fast path: -0.3 ms on space 4x, growing with the
  clock in play), the limb as `u*u*u` (-0.2).
* **A double-precision `atan2`/`asin` pair** (range reduction, minimax polynomials, |error| <= 2e-15 rad, ~6 ns against
  ~13) for the globe's and the band's longitude/latitude (`space_view.cpp:1026-1027`, `starfield.cpp:126-127`): 154 +
  45 serial samples at space 4x. -0.5-0.9 ms on space 4x, ~-0.5 on the telescope, -0.3 at 2x. The cost is 50-80 lines of
  a second trig implementation in `core/` beside libm, with a unit check over 1e6 directions and a note that it serves
  the per-pixel renderers only (a comparison with `std::atan2` elsewhere would differ by ulps); write it as plain
  `a*b+c` so that the six targets contract alike, or accept per-platform hashes as KI-014 does.
* **Optional raster reciprocals**: `invDyac`, `invDab`, `invDbc` per triangle and multiply (band-independent, no
  accumulation). Refuted and dropped: the 1/z subspan (an interpolation change that moves material contours by up to a
  pixel on near slopes: a look change) and per-row edge stepping from the band's first row (serial and parallel would
  differ in the last bits: B-310 broken).
* **The method**: `pngdiff` of the 22 regress frames and of the scene frames at 2x and 4x printing count, largest
  step and bank changes (expected: a handful of pixels by one unit, or none; a bank change is never accepted: the
  `atan2`/`asin` pair can flip a texel index at a material boundary, so it must show zero bank changes in the diff or
  stay `std::atan2`/`std::asin`); `cratered_noon` (the airless pow), `felisian_shore` (the
  day sky) and `space_S00` (the band) looked at; then `regress bless`. Serial against workers must still be byte-identical.

* **Done 2026-10-15** (`docs/decisions/log/4-1.3.1.md` F-10). The sky's powers as products, the clouds' drift wrapped once a frame and the limb's
  cube kept: no hash and no pixel moved, nothing blessed. Surface 4x 3.44 → 2.97 ms (-0.47 against the estimated -0.65, which was
  F-01's 8.8 ms frame's), forest 4x 8.92 → 8.37, space 4x 6.86 → 6.33 (the drift's and the limb's -0.5 as estimated), the telescope 2x
  8.95 → 8.64. The `atan2`/`asin` pair was built as planned and measured slower in the frame (space 4x +0.42 ms, the telescope 4x
  +1.2, Vimana +0.14): the 13 ns for libm was a random-direction loop's, whose signs defeat its branches; on a globe's scanlines libm
  takes 7.7 ns for the two against the pair's 5.7, and the pair's code in the globe's loop gave that back. Taken out. The raster's
  reciprocals not taken (an edge can sit exactly on a pixel's rounding).

### F-11. The close: budgets, docs, the regress widened (S)

Lower every `bench check` budget to about 1.5x the new measurement on this machine; add the F-01 rows' budgets where
they are stable; add a 1x cabin frame and the forest's serial-versus-workers check to what the harness guards; write the
numbers into `08-testing-and-tools.md`'s reference paragraph; narrow or close KI-312 (the terrain and the cabin), note
KI-331 unchanged (the planet function's cost is generation and out of scope), narrow KI-338/KI-403 by F-03's numbers,
KI-022 by F-02's; the `MILESTONES.md` F rows to done; `PROGRESS.md`.

* **Done 2026-10-15** (`docs/decisions/log/4-1.3.1.md` F-11). The budgets at about 1.5 times F-10's medians on this Mac (37 rows: seventeen lowered,
  twenty new; `bench check <factor>` elsewhere; the space and surface rows timed after five frames at the new scale, one run's first
  frames at 4x having read 11.0 ms against 6.4); the regress's forest at 4x and cabin at 1x, each drawn by one thread too. The series
  against section 2's targets: space 4x 13.6 → 6.33 (7), surface 4x 8.8 → 2.97 (4.5), forest 4x 14.8 → 8.37 (10), buggy 4x 11.9 → 4.04
  (8), surface 2x 6.6 → 1.82 (4), the landing map 2.29 (14), the descents 37 → 16.95 and 77 → 41.1 (20 and 55), with one thread space
  4x 40.6 → 26.65 (30), surface 4x 17.0 → 10.70 (12) and 2x 9.95 → 5.43 (7), and every stall's target met (a logged event's save 0.95
  ms, a photo's PNG and the recorder's on the writer's thread, a sprint's frames within 1.5 ms of its median); missed: the telescope
  2x 8.64 (8) and with one thread 24.95 (24), the forest with one thread 30.86 (28), the descent with one thread 63.8 (40). The
  telescope at its deepest at 4x, 25.0 → 18.33 ms, stays over a 60 Hz frame (KI-406). Released as 1.3.1 on 2026-10-15.

## 5. The order of work

One id at a time, in this order, each stopped after with its report (what was built, the checks with real results, the
frames worth looking at, the rows that moved, the next id), then the user's "proceed". Sizes as the project uses them
(S under two hours, M half a day, L one to two days, XL several days, verification included). The gains are the
sceptics' corrected figures at 4x unless a row says otherwise; they overlap where two items touch one pass, so the
targets of section 2 hold back a third.

| # | Id | Item | Size | What it buys (verified) | Pixels | After |
| --- | --- | --- | --- | --- | --- | --- |
| 1 | F-01 | Measure what the bench cannot see: the real game's frame sections, `pngdiff`, serial-versus-workers checks (cabin, forest, feeds, a 4x shore), per-band terrain trace, the pool and noise micro-benches, rows for the landing map, a sun-facing frame, the radar camera, the guide save | M | the proof tools and the numbers every later item is judged by; the flora's band question settled | none | |
| 1b | B-423 | The forest's picture depends on the core count: the flora's band layout pinned per scale (or one grid per frame by the serial rule: the user's choice) | S | B-310's contract restored for the flora; F-05 and F-06 provable | none on this machine with the pinned layout; the one-thread picture changes to match | F-01 |
| 2 | F-02 | The stalls in play: the probe records' lines cached in the guide, the PNG encoder, the PNG writer thread, the trees against a settlement's buildings, the direction table in parallel, the star neighbourhood walked incrementally, the landing probe's `initFrame`; the zoom build, `mapFor` and the re-anchor once F-01 has measured them (the user's call) | M-L | ~25-35 → 2-3 ms per log event, photo or autosave; ~17 → 6-8 ms per PNG and the stall off the frame; recording at 4x at 60 fps; tens of ms per frame gone in a capital on grass; six 2.3 ms hitches per sprint → 0.5; vimana 2x 2.5 → 1.2 | identical | F-01 |
| 3 | F-03 | The first frame of a descent and the tiles: the rings' misses filled by the pool, the tile's integer-key sort and queue swap | M | descent 37 → ~20 ms, beside a capital 77 → ~60, a re-anchor -30 ms; -6-12 ms per tile | identical | F-01 |
| 4 | F-04 | The terrain rings: cells binned by band once, water corners per vertex, the warm pass's vertex rows (the adaptive row chunks follow F-06) | L | the terrain section 5.2 → ~2.3-3.7 then ~2 ms at 4x, 4.2 → ~2 at 2x; the 4-to-8-thread slowdown gone; warm 0.8 → 0.5 | identical | F-01's trace |
| 5 | F-05 | The passes one thread draws alone: the cabin in bands, the map and menu blits, `glowDisc`, the camera feeds with the xorshift jump, the sky grids, the floor and reflections, the sun's cone test | M | space 4x -2.8; landing map 4x -2.5-3.5 (under budget); sun-facing frames -1-2.5; buggy 4x -1.0-1.3; cloudy frames -0.45; forest -0.55; space and surface 4x -0.25 | identical | F-01's checks |
| 6 | F-06 | The pool deals its rows: atomic chunks for the per-row passes and the plate, a bounded spin, the reentrancy guard, the terrain's adaptive chunks over F-04's lists; the cap raise as a measured step | M | space 4x -2 to -3, telescope -2, surface 4x -0.4-0.7, forest -0.5, dispatch latency -0.3-0.6 | identical | F-04, F-05, F-01's forest check |
| 7 | F-07 | Exact trims, in three or four commits: `wrap2pi`, the accessors inline, the globe's trims, the mush, the raster trims, the sky's skips, the halo and pillar tests, the plate's reach test, the probe table, the noise table, the synth constants, the flora riders, `ifloor`; the backdrop replay last | M | space 4x another -1.5-3 (the replay -1.1 parked), surface and forest -0.5-1, telescope -0.5-1, probe -0.3, a tile -5-12 ms, night and cold worlds -0.6-0.8 | identical | F-06 |
| 8 | F-08 | The platform layer: the audio block on a helper thread; the upload measured, a PBO only as an experiment | S-M | the periodic 33 ms frame at space 4x with music or the radar removed | identical | F-01's numbers |
| 9 | F-09 | The release flags: thin LTO on the clang lines, the scripts into `packaging/`, the Linux and Windows flags gated on the container's own regress | M | space 4x -0.8-1.7 on the Mac zips; Linux and Windows unmeasured | identical (clang); gated elsewhere | |
| 10 | F-10 | The hash-moving batch, one bless: the sky's pows as products, the cloud drift wrapped once, the limb, the `atan2`/`asin` pair | M | surface 4x -0.65, forest -0.7-0.9, space 4x -1-1.4, telescope -0.5 | visually identical, one bless | all above |
| 11 | F-11 | The close: budgets lowered, the regress widened, the docs and KNOWN-ISSUES | S | the gains guarded | none | F-10 |

Why this order. F-01 first because nine of the forty-three findings rest on estimates the bench cannot check, and
because its measurements already found B-423, which must precede any change of a band boundary (the guide,
the PNGs, the audio, the present, the map screens, the veil, the feeds, the capital's trees) and because every later
milestone is proved with its tools. F-02 and F-03 before the frame-rate work because a stall is felt before a frame
rate is: the plan's largest numbers are per event. F-04 before F-06 because the pool must not deal finer bands to a pass
that re-walks every cell per band. F-05 before F-06 for the same reason with the cabin and the blits (dealing helps a
pass only once it is banded). F-07 after F-06 because its gains are read against the dealt pool. F-10 last among the
code changes so that every other milestone is proved against unchanged hashes. F-09 any time (it touches no code);
F-08 when F-01's trace exists.

What a slower machine gets. The serial column of section 1 is the plan's proxy: F-04 (the terrain's leaner walk and
water corners), F-07 (`wrap2pi`, the accessors, the mush, the trims, the noise) and F-10 lower the one-thread frame
too (space 4x ~41 → ~30, surface 4x 17 → ~12); F-03 and F-02 lower every stall whatever the core count; F-05 and F-06
need cores. On a two-core machine the game runs at 2x today in ~10 ms and would run in ~7.

## 6. The method, every milestone the same

1. **Measure before.** `./vesperis_test bench check` three times at rest (the rows move by about 5% between runs; take
   the median), `VESPERIS_THREADS=1 ./vesperis_test bench` once for the serial baseline, and `./vesperis_test regress`
   green. Note the rows the milestone targets.
2. **Change one thing.** The milestone's change and nothing else in the same commit; no second code path, no toggle.
3. **Prove the pixels.** `./vesperis_test regress` must stay green for every bit-identical milestone (the 22 hashes
   cover four frames at 1x and the generation). The serial and the parallel run must agree: `VESPERIS_THREADS=1
   ./vesperis_test regress` and the default run hash the same frames (B-310's rule); for the scaled frames, which
   `regress` does not cover, the `pngdiff` mode of F-01 compares the `scene` frames at 2x and 4x and the `bench_*.png`
   of a run before the change with a run after: zero differing pixels for a bit-identical milestone. A hash-moving
   milestone (F-10) prints its differing pixels and their largest intensity step, which must stay within one
   stop (32 units of 2048) and be reviewed in the images, then blesses the baseline once with the reason in
   `docs/decisions/log/4-1.3.1.md`.
4. **Measure after.** The same rows, three runs; the result goes into the milestone's row in section 5 and into
   `08-testing-and-tools.md`'s reference numbers. A milestone that does not deliver at least two thirds of its
   estimate is reconsidered before the next one starts.
5. **Tighten the budgets.** When a row falls, `bench check`'s budget for it falls with it (to about 1.5x the new
   measurement on this machine), so a later change that gives the gain back fails CI.
6. **Record.** A row in `docs/decisions/log/` for every structural change (why, consequence), the numbers in
   `08-testing-and-tools.md`, `KNOWN-ISSUES.md` for what remains (KI-312 closes or narrows), `PROGRESS.md`'s log.
## 7. Implementation checklists for the nine largest items

A second reader went through the code each of these touches and listed what it relies on, what could break and how to
prove it did not. Every one came back "go" or "go with care"; the independent gain estimates sit beside the sceptics'.
The checklists are the parts an implementer is most likely to get wrong.

**F-02 · the guide's probe lines** (go with care; ~20 ms per save; ~3 h). The cache is built with an `ostringstream` at
`precision(12)` and nothing else (a default stream truncates to six digits: the `cmp` catches it); `save` never trusts the
cache's presence (`if (r.lines.empty()) r.lines = probeLines(r);`, a `mutable` member) so a record made by a future path
still writes its lines; `load` pre-fills after the whole file is read (filling at the `probe` header would cache empty
tracks); the `probe` header line keeps `lent` and `saved` live; every save reached outside `Game::frame` (the migration
save in `loadFromDisk`, the quit path, the explicit harness saves) stays direct if the dirty flag is ever added; a comment
at the four vectors says they are immutable once made. Test: the scratch program's `cmp` of the file saved by old and new
code from the same loaded guide, after a load-save-load-save chain and after an inbox import; `unit`'s X-04 and lending
checks; `flow`'s guide-reloaded line.

**F-02 · the PNG encoder** (go; ~8 ms per 4x image; ~2 h). NMAX is 5,552 or less (the bound: `b` after `n` bytes is at
most `65520(n+1) + 255n(n+1)/2`, which fits `uint32` for 5,552 and not for 5,553); both sums reduce at the end of every
run and once more before packing; the chunk CRC seeds the data CRC with the type's running value (`crc(data, n,
crc(type, 4)) ^ ~0`, one complement, the length bytes excluded; IEND's CRC must come out `0xAE426082`); the three pixel
byte expressions stay as they are through a `uint8_t` pointer after the row's filter byte; keep `raw` and the block loop
(rows of 3,841 bytes do not align with 65,535-byte blocks); `if (n) fwrite(...)` for the empty IEND; `#include
<algorithm>` explicitly; the lazily built CRC table becomes a function-local `static const std::array` (thread-safe
initialisation, ready for the writer thread). Test: `cmp` of every PNG that `flow`, `unit` and `bench` write against the
previous binary's, plus a fifteen-line Python check of every chunk's CRC and a `zlib.decompress` of the IDAT, because
`readPNG` verifies neither checksum and the gallery would show a corrupt file happily.

**F-02 · the PNG writer thread** (go with care; ~7 ms per event after the encoder fix; ~4 h). A function-local static
`PngWriter` (the `WorkerPool::instance` pattern) whose destructor drains and joins, plus an explicit `pngFlush()` before
`platformShutdown` and at the end of the harness's `main` (`--frames N` exits right after queueing its screenshot); frame
buffers recycled through a free list (a fresh 4 MB vector per frame costs ~1 ms of page faults); jobs carry their own
`w, h` (`FBW/FBH` change with the scale); names and counters stay on the main thread in call order; `readPNG` and
`openGallery` flush first (the gallery lists the directory); both waits use predicates; Emscripten falls back to the
synchronous call in one `#ifdef`. A behaviour change to note in KI-022: recording at 4x stops halving the simulation's
frame rate, so a movie holds twice the frames per game second. Test: `cmp` of the flow's take frames and shots; `unit`'s
X-04 gallery check; the TSan scratch build; `./vesperis --frames 3` writes its screenshot; memory flat over 30 s of
recording.

**F-03 · the rings' first fill on the pool** (go with care; ~16 ms off the first frame; ~5 h). Never call `mesoFor`
inside the parallel region: today the serial fill guarantees every material's meso tile exists before the band threads
run, so the fill collects a 64-bit material mask (over hits and fresh cells alike) and builds the tiles after the join; a
unit check asserts `2 x ringR[k] + 3 <= 1 << logN` for the four rings and `2 x ringN + 3 <= 1 << lodN.logN` on a felisian
and a small-body site (today 83/75/55/55/63 against 128/128/64/64/256: distinct cells, distinct slots); both paths (serial
under ~64 misses, parallel over it) iterate one `forDisc` helper with the bounds verbatim (`rFill2 = (r + 2)^2`); the
misses are counted in the serial pre-pass only (no member written inside the region); the pool's join is the barrier (no
atomics, no overlap with the vertex pass); leave `prefetch(budgetMs)` serial in the first version (budgeted, invisible,
done within seven descent frames); measure the cold-landing case (several workers hitting missing drainage tiles, each
`buildTile` spawning its samplers) before bounding it; watch RSS: each pool worker pins up to twelve drainage tiles
through `t_tiles` after a parallel fill (a release call dispatched to the workers if it shows). Expect 37 → ~21 ms, not
15. Test: the `warm NNNN m: N misses, fill X ms` trace lines; `regress` (the three scene frames are first renders of
fresh views, so they exercise the parallel fill directly); a fresh-site serial-versus-workers unit check comparing the
framebuffer hash and the five caches' bytes; `capsule` 0.00%; a scripted re-anchor.

**F-03 · the tile's sort** (go with care; ~5 ms per tile; ~3 h). Build `keys[q] = monotone(hc[order[q]]) << 32 | q` over
the flood's pop order (every cell is pushed and popped exactly once), with `-0.0` canonicalised to `+0.0` before the
monotone bit mapping (the comparator treats them equal), sort ascending and read `topo[k] = order[(uint32_t)keys[n - 1 -
k]]` (the comparator wants `hc` descending then pop index descending); a `std::sort` of 176k `uint64` is 3-6 ms here, so
the gain is -3 to -7 ms per tile; the breach heap's clear through a derived `Heap { void clear() { c.clear(); } }` (a fresh
`priority_queue` per pit would allocate thousands of times); scope the key vector so it is freed before the propagation
loops; leave `buildTiles`' sampler sizing and `MARGIN_DEG` alone. Step 0 before any edit: fold the finished tile's
`lvl/acc/dir/up/flag` bytes into one hash printed in the trace line: the decisive before-and-after test over `drain`,
`drain desert`, `drain thin_atmosphere`, `home` and `scene felisian_shore`; the bench's descent rows cannot move (they
time frames after the tiles are ready), so never bless for this.

**F-04 · the terrain cells binned once** (go with care; ~2.2 ms at surface 4x; ~11 h). One source of truth for the band
bounds: factor `parallelBands(n, minPerThread) -> {threads, size}` out of `parallelFor` and use it in both the bin and
the dispatch (a thread count that differs after a scale change or under `parallelThreadsOverride()` would bin into the
wrong rows); one list per band appended during one serial walk in ring/row/column order and drawn start to end (no
sorting by distance, no de-duplication: depth ties at shared edges go to the first writer and must stay so); the bin
calls `vertexOf`'s hit path for the four corners of every listed cell so no stale vertex is drawn and no band thread ever
writes a cache (keep the warm's serial fill and its `mesoFor`); the hull rows take `[ceil(ymin - 0.5) - 1, floor(ymax -
0.5) + 1]` over the ground corners, the water corners of wet cells, the corners raised by `SHORE_LIP` for cut cells and
the wall bottoms of the outermost ring's edge cells; any corner behind `NEAR_Z` and every foam cell go to every band; the
bin never reads `VESPERIS_SKIP`; lists are reused members of 12-byte entries (~120 KB); the bin's ms is printed under
`VESPERIS_TRACE` and lands inside `section(2)` so the bench compares like with like. Step 1 bins with today's predicate
semantics (wet, edge and not-in-front cells to every band, the loose slack); step 2 tightens to the hull rows, drops
empty ranges and the inner rings' edge escape; the adaptive row chunks are a third step and need F-06's dealing primitive
(`WorkerPool::run(count)` spawns `count - 1` workers: 32 chunks today would mean 31 threads). Test: `regress` twice (the
default pool at 1x is already six bands of 34 rows, so a band-edge hole in the three scenes fails it) and with
`VESPERIS_THREADS=1`; the shore, forest and a small-body scene at 4x and 2x through the unit's both-ways check; the
thread sweep at 1x (the 3.8 → 5.4 ms slowdown must be gone).

**F-05 · the cabin in bands** (go with care; ~2.6 ms at space 4x; ~5 h). No `return` inside the band body: the roof
branch's early return (`cabin.cpp:187-192`) would leave `job(0)` on the main thread with a raster band still set and every
later serial draw clipped to it; the bulb's and orb's `glowDisc` are not band-clipped and additive, so they stay in a
serial tail after the dispatch, gated by the roof flag; `solid`, `rp`, `rs` and the four lambdas are declared inside the
body (the `screen` lambda writes `rs.bank` today); `shardsPending()` (a `std::map` and a `std::set` per call),
`droneBayReady()`, the rack places and poses and the flicker scalars are hoisted and captured by const reference; the
hull's quads, not the rack's, are 70% of the cost (the linker folded two identical lambdas into one symbol, which misled
the finders); if space 1x regresses, build the three drones' primitive lists once above the dispatch. The equality check
must render the same instant twice: `Game::frame` advances `realTime` and the screens flicker on it, so re-render with
`frame(in, 0)` or a `testRenderSpace()` accessor, and reset `parallelThreadsOverride()` and the scale on every path out.
Expect space 4x 13.6 → 10.4-11.3 ms; the `VESPERIS_THREADS=1` rows unchanged (the serial path pays nothing for the
dispatch).

**F-05 · the maps' and menus' blits** (go with care; ~2.6 ms landing map, ~2.0 sector map, ~0.75 per menu frame at 4x;
~5 h). `blendRectRGB`'s three statements stay verbatim (`(cr - r) * a / 255` is a signed truncating division: a
"faster" `>> 8` or an unsigned form changes values); the dispatch threshold is 100 rows as `toRGB`'s (the 48-row status
bar and the thousands of 1x1 blends stay direct calls); the whole-world blit uses the memo form (compute a texel's colour
when `tx = px * W / PW` changes, copy the row above when `ty` repeats, `lastTy = -1` at each band's start so no band reads
another's row), never a block fill (at 1x every second texel is skipped and at 3x they alternate); the sector map's
`ix/iy` tables are rebuilt every frame (`shiftX/shiftZ` move) with the `(int)` truncation kept; the zoom roads, the frame,
the peoples' tint and the cursor stay after the blit, in order. Add bench rows for the landing map (world and zoom) and the
sector map at 4x, and a unit check hashing `game.output()` for both screens with the pool and with one thread. Test: `cmp`
of the flow's landing, zoom and sector frames at every scale (3x is where texels alternate), and the nesting check (a
`flow` at 4x must finish: a nested dispatch would hang).

**F-06 · the pool deals its rows** (go with care; ~2.3 ms at space 4x; ~10 h). `generation`, `pending` and the chunk
counter become atomics with release stores and acquire loads (spinning on plain `int`s is a data race the compiler may
hoist); complete a dispatch by work, not by head count (an in-flight counter: a participant descheduled before it wakes
must not gate the join while the others finish its rows); `parallelBands(n, bands, fn)` keeps exactly `bands` chunks for
the terrain, forms and flora with the counts pinned by named constants (`FLORA_BANDS = min(8, FBH / 32)`: six at 1x, which
is what the regress hashes today); the serial path runs the bands sequentially so the layout is thread-count independent;
the spin is bounded (~20-40 µs by `steady_clock`) and only right after a job; a `thread_local` in-job flag makes a nested
`parallelFor` run serially and a `try_lock` makes a second caller run serially too; the probe's chunk is clamped so a
caller with `minPerThread = 1` still gets small chunks; the harness's per-band statistics keep their meaning because the
flora's bands are pinned; the cap raise is its own commit after B-422's core counts are re-read. A new property worth a
unit check: `VESPERIS_THREADS=2, 4, 6, 8` give identical PNGs on six scenes at 4x. The sky's gain is 0.4-0.9 ms, not
1.0+ (its remaining serial parts are the star field, the grids and the palette).

## 8. Considered and set aside

Each with the reason, so the question is not reopened without new evidence.

* **Background thread priorities (QoS)** for the drainage, plan, text and map threads: refuted. The worst frames cited
  as evidence came from a contaminated run (`bench_par2`: every row slower) or from known periodic costs (the probe's
  passes, KI-361); the clean run's landing worst is 5.4 ms against a 5.2 mean. B-422 measured and refused slower
  prefetches (a landing waited longer); macOS does not boost a utility-class thread for a plain join. If worst frames
  ever matter, prove the cause with `sample`/spindump during the landing and probe rows first.
* **Drawing the sky after the ground** to skip the covered half: no exact formulation exists (the band pass's block test
  reads the block's first pixel before the ground; a moon's limb at the horizon; the lens flare's gating), so it would
  change the night horizon: a look change, refused. The exact items (the pows, the skips) take most of the gain.
* **The flora primitive list** (cells generated once, trees placed once, drawn per band): the tree part's serial
  coverage pass eats its gain and changes which trees are hidden (hash-moving), the cells part is worth ~0.15 ms for an
  XL rewrite describing the picture a second time. Only inside a future dynamic-bands milestone for the flora, where
  the repetition would grow with the band count. The two trivial riders are in F-07.
* **The 1/z subspan and per-row edge stepping in the rasteriser**: the first is an interpolation change (material
  contours move by up to a pixel on near slopes), the second breaks serial-equals-parallel in the last bits.
* **Splitting primitives among threads** instead of rows: a race on the depth test; rejected at B-310 and still.
* **`-march=x86-64-v2`** for the x86-64 zips: SIGILL at start-up on pre-2009 Intel and pre-2011 AMD CPUs; `ifloor`
  takes the one hot case portably.
* **PGO, `-O3` on the Mac, `-fno-plt`, `-no-pie`, `-fno-stack-protector`, a newer Ubuntu base**: `-O3` measured as
  noise; PGO is a training run on six targets for an unmeasured gain; the micro-flags make the lines diverge; a newer
  base loses glibc compatibility. Thin LTO is the one flag kept (F-09).
* **Splitting `main_headless.cpp`** (18,300 lines): compile time only, not game performance.
* **The `toRGB` palette LUT cache**: <=0.05 ms for an invalidation rule.
* **Sizing the drainage samplers to the lanes** in a synchronous landing build: B-422 measured and refused it (a
  six-tile landing on six threads leaves four cores idle).
* **The post-order walk for the tile's accumulation**: not a sort replacement but a generation change (a breached
  basin's catchment would start to flow downstream); the integer-key sort gives the same permutation.
* **Caching the dimmed scene under the maps, the cabin's pixels across frames, the cloud grid across frames, the
  panoramas across frames**: none identical (the clock runs, the globe turns, the head bobs, the wind drifts).
* **Drawing the cabin, the veil or the maps at 1x and upscaling; a coarser sky step; fewer trees or cells; a lower plate
  budget**: look changes, all refused.
* **Re-seeding the camera feeds' noise per row from a hash** instead of the exact jump: a different speckle; no hash
  covers the feeds, so it would pass unnoticed, which is the reason not to.
* **A per-texel slope map for the globe** in place of the four `heightAt`: a different estimator at the texel scale.
* **A faster fractional `pow` for `exposureStop`, float in the globe's lambda, a table for `pow(1 - el, 2.2)`**: errors of
  1e-6 and more, or a different terminator; refused or left for a later batch.
* **A wall-clock plate budget, sampling the plate on a thread of its own**: the sharpening's pace would depend on the
  machine, or the frames in which cells appear would change.
* **The lazy lat/lon in the plate's pixel path**: a flag a future reader must honour before every use, for a gain the
  bench never sees.
* **Folding `clearDepth` and the space clear into the first pass**: ~0.02 ms for three files owning one buffer's clear.
* **A pixel-unpack-buffer ring for the texture upload** before measuring: a second upload mechanism with per-OS headers
  and a WebGL fallback for an expected 0.1-0.2 ms.

## 9. Risks and open questions

* **Most of the per-event numbers are analytic.** No sample ever caught `Guide::save`, `writePNG` on a photo, the
  audio block, the texture upload, the map screens or the veil: their costs are operation counts calibrated on
  neighbours. F-01 exists to replace them with measurements before the work; a finding whose F-01 number comes in under
  the bar is dropped, not built.
* **The regress is narrow and the picture is wide.** Four frames at 1x hash nothing of the cabin, the forms, the flora,
  the feeds, the screens or any 2x-4x frame. Every item here is proved by `cmp` of harness frames against the saved
  previous binary and serial against workers; the one-line rule for an implementer is: a `regress` that passes proves
  only what it hashes.
* **The flora's band layout.** If F-01's forest check shows the parallel forest differs from the serial one today, the
  fix (one coverage grid by the serial rule) moves a few canopy pixels and needs a bless; until then the flora's band
  count is pinned and nothing in F-06 touches it.
* **Codegen on the pixel path.** B-411 recorded a compare added before every opaque pixel costing 3-10 ms; F-05's cabin
  body, F-07's `emitPixel` split and the sky's guards all touch hot loops and are judged by alternating A/B pairs, with
  the surface, forest and city rows as controls.
* **Machines that are not this one.** Spinning workers (F-06) and a tenth busy thread (F-02's writer, F-08's synth) are
  gains on ten cores and may be losses on four; the serial column is the proxy for a slow machine but not for a
  four-core one. The release containers can run `bench check` on arm64; a four-core run is an open item.
* **The cold landing.** F-03's parallel fill can send several workers into `buildTile` at once when the site's drainage
  tiles are not built (KI-338/KI-403); whether the builder storm costs more than it saves is measured, not assumed.
* **Memory.** F-03 pins drainage tiles per pool worker, F-04 adds ~3 MB of water corners to the vertex caches, F-07's
  backdrop layer 2 MB, F-02's writer three frames of 4 MB; the bench's peak is 135 MB today and is re-read after each.
* **Two generation questions surfaced and are not for this plan**: a breached basin's catchment does not flow downstream
  in the drainage tiles (the post-order walk would have changed that: a GEN bump), and the cloud read's `fmod` grows
  with the clock until F-10 wraps the drift once a frame.
* **The web build** has no measurement at all (KI-021); the single-threaded column stands in for it, and F-02's and
  F-08's threads stay off under Emscripten.
* **What the plan does not touch**: the planet function's cost per sample (KI-331, generation), the drainage's margin
  and tile size (routing changes), the plate's budget (a feature), the look.
* **The probe's handover** (KI-361: 4.2 s at 20-25 fps at 4x at every launch, the space frame and the probe's three
  passes drawn together over 62-92% of the entry) is the longest sub-30 fps stretch a player meets at the default scale;
  the cabin, globe and dealing items shave their share, and what remains is a look decision (the handover's passes at
  the logical resolution, as X-02 already draws the panoramas), not an item of this plan.
* **The guide grows with play**: F-02 removes the per-save formatting, but the file and its load at start grow with
  every probe flight (five flights, 0.57 MB of camera keys today); a cap or a thinning of the tracks is a design
  question for the user.
* **The thread budget as a whole**: B-422 sized the drainage lanes at cores minus two and refused half the cores for a
  worker; a spinning pool, a tenth busy thread and a raised cap all draw on the same cores in the first second after a
  landing. The balance comes from counted threads per state (F-01), never from priorities (refuted).

### What the completeness critic found

Twelve gaps, now placed: the forest's band dependence (measured, B-423, first); the landing map's zoom build, `mapFor`
and the re-anchor (F-01 rows, F-02's loading items); the probe handover and the guide's growth (above); the forest
bench's kernel (F-01, F-11's budgets at the game's kernel); the thread budget (above, F-06); the warm fill's meso-tile
hazard (F-03's checklist); the binning's exactness conditions and the water corners first (F-04); the backdrop cache's
key completeness and its idle-only gain (F-07, optional and last); the audio item as a dropped frame and the vsync
interplay (F-08); the hash-moving batch's bank-change acceptance (F-10). Its ordering advice is this plan's, with one
emphasis kept: the dealing (F-06) is worth doing for the per-pixel passes before the terrain's binning if F-04 slips,
since those passes carry no per-band state.

## 10. How the numbers were taken (to repeat them)

All on the dev Mac at rest, `make -j8` of `release/v1.3.1` (the 1.3.0 code), in this order so that no two measurements
shared the cores:

* `./vesperis_test bench check` (eight workers) and `VESPERIS_THREADS=1 ./vesperis_test bench` (the serial baseline),
  then `VESPERIS_THREADS=2`, `4` and `6` for the scaling curve; the rows and the sections are the tables of section 1.
* Profiles with macOS `sample` at 1 ms: `./vesperis_test bench forest` with workers (3 s) and serially (6 s, which
  includes the scene's set-up); the whole `bench` with workers in one aggregate (the run lasts ~11 s; `sample` was asked for 60 and stopped with the process); the whole `bench` serially under a
  pseudo-tty (`python3 -c 'import pty,sys; pty.spawn(sys.argv[1:])'` so that the rows print as they finish) in 2 s
  windows, each window matched to its row by the timestamps (window 1 = space 4x, 2 = telescope 2x, 4 = surface 4x/3x/2x).
  The "Sort by top of stack" list of each sample gives the leaf costs; the call graph of the main thread gives the
  paths.
* The flag experiments: the tree copied to a scratch directory and built with `make -j8 vesperis_test CXXFLAGS=...` for
  `-O3`, `-O2 -flto=thin` and `-O3 -flto=thin`, each binary run through `bench` and `regress` from the repository
  directory, the `-O2` binary run again at the end as the control.
* Peak memory of a `bench` run: 135 MB (`/usr/bin/time -l`).
* The forest equality test: `./vesperis_test scene felisian_forest 4` and the same with `VESPERIS_THREADS=1`, the two
  `shots/tests/scene_felisian_forest.png` compared with a thirty-line Python reader of the harness's PNGs (stored
  deflate, filter 0: `zlib.decompress` of the IDAT, then a per-pixel compare printing the count, the largest channel
  step, the rows and the per-eighth distribution); the same for `felisian_shore` and at 2x and 1x;
  `VESPERIS_THREADS=1 ./vesperis_test regress`.

What is not in the numbers: the real game's window (`platformPresent` uploads a 1280x800 RGBA frame to the GPU every
frame and runs the post shader), its audio pump and its input polling, which the headless harness does not run;
machines other than this one; the web build (single-threaded unless the host sends the COOP/COEP headers, so the
serial column is its reality).
## 11. What the docs get when a milestone lands

The project's five touch points, as the constraints reader listed them from the tree:

* `docs/decisions/log/`: a dated section per milestone (`## F-NN (date): title`) of bold-led bullets: the
  decision, why, what was tried and dropped with its measurement, the price; ending with what moved (no generation
  change, GEN_VERSION 13 holds; the regress unchanged, or re-blessed and why). F-03 amends M7-01's rule (the caches
  written by the pool inside a joined region on disjoint slots); F-08 amends the audio contract; F-10 records the bless.
* `docs/reference/08-testing-and-tools.md`: the mode table (`pngdiff`, `bench noise`, the new rows), the `bench` row's
  budgets, a dated "After F-NN" paragraph of reference numbers (the convention of the existing ones), the regress
  paragraph when a frame is added or blessed.
* `docs/reference/02-rendering.md` (the mush's definition and kernels, the band-parallel paragraph, the draw order, the
  backdrop replay's key, the cabin in bands), `06-surface.md` (the warm pass, the binned rings, the prefetch, the per-band
  trace), `01-architecture.md` (the three kinds of worker, the present/audio path), `04-galaxy-and-systems.md` (the
  neighbourhood's incremental update), `05-planet-function.md` (the tile's sort), `07-game-flow-and-controls.md` only if
  a setting changes (none planned), `09-cookbook.md` (a recipe: profiling a scene with `sample`, the A/B pairs).
* `docs/iteration-3/bugs/KNOWN-ISSUES.md`: KI-312 narrowed or closed, KI-331 kept, KI-338/KI-403 narrowed by F-03,
  KI-022 by F-02, KI-206 by F-04/F-07; a new row for anything a milestone leaves (the forest's horizon band, the loose
  sun box, the cloud read's `fmod` growing with the clock until F-10).
* `docs/archive/iteration-3/plans/PROGRESS.md`: a dated bullet per milestone with the checks' real results (unit N 0 failures,
  regress 22 unchanged or re-blessed and why, flow seconds, bench check within every budget with the rows that moved);
  `docs/ideas/MILESTONES.md`: the F series' status column; `tests/regress_baseline.txt` only through `regress bless`.
* `CLAUDE.md` and `docs/README.md`: the new harness modes and the trace line, nothing else.
