# B-423: the forest's picture depends on the core count

status: resolved 2026-10-12 (built and proved on the pixels; the timing pairs and the flow frame closed 2026-10-13, see the end)
severity: S3
area: the flora (`surface/flora.cpp`), the worker pool (`core/parallel.h`)

## Report

Found by the performance audit's last measurement (`docs/ideas/PLAN-performance.md`, section 1) and measured again by F-01's
`bothways`: `scene felisian_forest` drawn with `VESPERIS_THREADS=1` against the default eight workers differed by 13,921 pixels at 4x
(1.36% of the frame), 3,419 at 2x and 506 at 1x, with channel steps up to 130: far trees shown in one run and hidden in the other, in
the canopy rows of the middle of the frame (`shots/tests/bothways_felisian_forest_4x_diff.png`). The shore and the regress's three
scenes were identical both ways. B-310's contract (a frame drawn by the workers is the serial loop's) was broken for the flora: a machine
with four cores drew a different forest from the dev Mac's, and no band-boundary change of the performance plan (F-05, F-06) could be
proved on the flora.

## Cause

The flora's canopy-coverage grid (N2-04: a far tree whose canopy falls wholly on tiles already filled by nearer canopies is skipped) is
kept per band: each band thread marks its own grid with the clusters it draws, the ones that pass its band test, while `covered()`
reads every tile under a canopy. A far tree whose canopy crosses a band edge is hidden in one band layout and drawn in another, and
the layout followed the thread count (`parallelFor(FBH, 32, ...)`: six bands at 1x and eight at 2x-4x on the dev Mac, one with
`VESPERIS_THREADS=1`, four on a four-core machine).

## Fix

The plan's recommended way (its section 4, option 1), taken on the user's "proceed": the flora's band layout is pinned per scale.
`parallelBands(n, bands, fn)` (`core/parallel.h`) runs exactly `bands` contiguous bands whatever the threads, shares them out among the
threads and runs them one after another on a single thread; the flora runs `floraBands()` = min(8, FBH / 32) of them, six at 1x and
eight at 2x-4x: the dev Mac's layout. The per-band grid stays what it was, an approximation of the canopy cull that a band edge can
cross, now the same on every machine. The other way, one coverage grid a frame by the serial rule, would have moved the forest on the
dev Mac (a few far trees that show through canopy holes today hidden) for a serial pass of about 0.2 ms; it was not taken.

What moved: nothing with eight threads or more (the regress, and the scene frames at 1x, 2x and 4x with the default threads, identical
to F-01's binary); the one-thread picture moved to the workers' (every felisian scene with trees: the forest, the tropical biome, the buggy's seat; the
other scenes identical); a machine
with fewer threads now draws the dev Mac's forest. `unit`'s both-ways check holds the forest at 1x, 2x and 4x with one, three and eight threads.

## Verification (2026-10-12)

Against F-01's binary: `regress` 22 unchanged, and with `VESPERIS_THREADS=1`; `unit` 320 checks, 0 failures (the both-ways check 20
frames: the forest at 1x, 2x and 4x with one, three and eight threads); `bothways felisian_forest` identical at 1x, 2x and 4x. The scene
frames of eight scenes at 1x, 2x and 4x: with the default threads all 24 identical to F-01's binary's; with one thread nine moved to the
workers' picture, the forest (506, 3,419, 13,921 pixels), `biome_tropical` (278, 1,574, 6,327) and `buggy_seat` (204, 1,078, 4,521):
B-423 reached every felisian scene with trees; B-423's binary draws all 24 the same with one thread as with eight. The flow from the
same starting files: F-01's binary twice identical (163 of 163 pictures); B-423's differs in one, `flow_probe_aim.png` (38,171 pixels),
a space frame the flora never draws into, which also varied between runs in F-01's measurements.

Open when the work stopped (the user's "write down progress"): a second B-423 flow run from the same state to settle
`flow_probe_aim.png`; the four alternating `bench check` pairs against F-01's binary; the serial cost of the pinned bands (one thread now
walks eight bands' trees in the forest: `VESPERIS_THREADS=1 bench check` and `bench forest` against F-01's binary).

Closed 2026-10-13, with F-02's runs. The flow from one restored state in eight runs (B-423's binary four times, F-01's once, F-02's
three times): `flow_probe_aim.png` the same in all eight, so its difference on 2026-10-12 was the flow's own (B-423's binary differed
from itself in 45 pictures in one of its four runs; KI-404). Four alternating `bench check` pairs against F-01's binary: every row within
the runs' spread (space 4x 14.11 against 14.06 ms, surface 4x 8.87 both, forest 4x 15.53 against 15.15). The serial price of the
pinned bands: `VESPERIS_THREADS=1 bench forest` 39.7-40.3 ms against F-01's 37.8-38.7 (the flora 14.6-15.6 against 12.6-13.3), and in
one serial `bench check` each the forest 2x 22.3 against 19.2, 4x 39.5 against 37.2, at the game's 3x3 40.5 against 36.5; every other
serial row the same.
