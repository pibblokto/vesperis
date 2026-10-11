# B-422: freezes of five to seven seconds at a landing, at Tab and R on the ground, and at the landing map's Enter

status: resolved 2026-10-11
severity: S1
area: the surface (R-408's marks, the descent's camera), the landing map, the drainage (O6-03's tiles)

## Report

The user, with C-18 to R-416 committed: "there are two lags that happen occasionally. First one - when you land and get out of the
capsule you lag into poisiotn with looking slightly underground and everything is frozen for about 5-7 seconds (screenshots attached)
[...]. It seems to be more prevelant on felisian/desert planets but besides that I didn't find any other logic. ANother freeze I get is
when I either open surface metrics when on surface with tab immediately when I exit the capusle or I press R to enable scanner. It is
more rare but also gives annoying 5-7 seconds freeze. Also other two things, they are also quite important but not as much - when you
are on the stardrifter and move cursor on the map while selection a sector some users report CPU spiking. What I also noticed is that
when you press enter game occasionally also freezes for about 5-7 seconds". The screenshot: `SURFACE CAPSULE DESCENDING`, the ground a
slanting band with the sky's colour under it.

## Cause

Reproduced with the user's save and guide (copies) on their world BUBAS, each frame timed at 30 a second (`lagflow`, `lagmap`, scratch):

* **Tab, R and the landing itself**: R-408's marks of the surface's world (`Game::marksHere`, rebuilt at every landing) read each mark's
  ground at the map's 64 m with the rivers, and a mark far off stands on drainage tiles nobody had built (a tile of a degree, 0.2-0.8 s
  of one core's routing after its samples): a tile was built on the main thread for every mark across the world. The user's guide holds
  nineteen marks on BUBAS. The readings' rangefinder line asks for the mark the crosshair rests on (R-414, the readings open across
  landings), the scanner for the marked echoes (`echoMarked`), a shown mark (B-412) and the N map for theirs: the first frame on the
  ground with the readings open took 8,492 ms, the third after R 8,121 ms. The drainage is a felisian, desert, thin-atmosphere,
  hydrocarbon or acidic world's, and the marks of ruins a people's world's: hence the felisian and desert worlds.
* **Slightly underground**: the frame the freeze held was the descent's last; its camera came down to the ground itself
  (`cameraOverrideAlt` to 0, the standing eye 1.65 m over it): for the descent's last third of a second the eye stood under the near
  ground's triangles and saw the sky through them.
* **The landing map's cursor and its Enter** (its title is `SELECT LANDING SECTOR`): O6-03 asked for the drainage tiles of every place
  the cursor stepped to (65 km round it, six to nine tiles) on a thread of its own, each tile sampled by eight threads; a later asking
  dropped an earlier one's remaining tiles but not the one it was computing. Shift held a second (six degrees a frame): 46 threads, 8.2
  cores of 10 busy and the frames slowed to 140-190 ms; arrows tapped every 0.15 s: 5.8 cores (the machine otherwise idle). The Enter
  after them built the site's own tiles on the main thread one after another among the computations for the places passed: 2,951 ms
  (701 ms tapped; 1.2-1.5 s with the map just opened).

The star map and the galaxy chart were measured too: 0.03-0.72 cores, no frame over 60 ms, the jump's flight none.

## Fix

* **The marks** (`game/marks.cpp`): a mark's ground is read with the rivers only where their tiles are built (`drainageReady`), else
  without them (`DrainageOff`, as the scanner's sights' heights already were); a mark read so (`SiteMark::dry`) asks again every two
  seconds and is read with them once the explorer's way has built its tiles. Only the height of the shown mark's diamond over the view
  reads it.
* **The descent** (`SurfaceView::render`): the camera stops at the standing eye, `max(cameraOverrideAlt, eyeHeight)`; the ascent starts
  there.
* **The landing map** (`Game::updateLandingMap`): the cursor's tiles are asked for once it has rested 0.4 s (`landSeenLat/Lon`,
  `landMovedT`); the default site's at once.
* **The drainage** (`galaxy/drainage.cpp`): a prefetch's tiles are built side by side, a lane each, the nearest the point first
  (`buildTiles`); a worker keeps to every core but two (a lane for every four, the tile's samplers sharing the rest), a caller that
  waits (`SurfaceView::init`, `reanchor`) takes every core but one. A later prefetch supersedes an earlier worker's remaining tiles as
  before, and the tile it was computing is abandoned at its next checkpoint (every four rows of its samples, every 4,096 cells of its
  routing) unless a thread waits for it or the newest prefetch asked for it too (`Abandon`, the cache entry's `Interest`): a tile is
  computed whole or not at all, and the same tile is the same as before. `drainageTilesBuilt` counts the tiles computed, for the tests.

## Verified

Each before and after measured the same way on the same machine (another Vesperis process ran beside both: the numbers are its, not an
idle machine's), the before from the commit `c880985` built in a worktree:

* BUBAS with the user's save and guide, landing 2 km off the capital after 3 s on the map: Tab on the first frame on the ground 7,767
  ms before (8,492 on an idler run), no frame over 60 ms after (the run's worst 99 ms, the Enter's); R 8,289 ms before, none over 60
  after; the N map opened and zoomed out to 2,048 km, none over 60. The descent's last frames stand at the eye and the first on the
  ground matches them (before: the near ground smeared flat under the eye, `descent_compare`, scratch).
* The landing map on BUBAS (frames paced at 30 a second): arrows tapped every 0.15 s, 7.6 cores, 20 threads and the frames late (8.1 s
  of wall for 6 s of frames) before, 1.6 cores, 8 threads and on time after (the map drawing itself, the space view behind it, takes
  1.6-1.7); tapped every 0.3 s, 6.7 cores before, 1.6 after; every 0.5 s 5.3 and 5.1, every second 3.5 and 3.5 (each rest of 0.4 s
  asks for its place, as the landing wants); Shift+Right held a second, 8.2 cores and 46 threads before (an idler run), 1.6 and 8 after.
  The Enter right after the tapping: 2,439 ms before, 1,020 after; after the held cursor 2,951 before, 936 after; 2 s at rest first, 77
  ms. The Enter as the map opens on four worlds: 1,207, 1,701, 837 and 437 ms before, 723, 943, 573 and 616 after.
* `unit` 0 failures (318: three new), `regress` unchanged (22 hashes), `flow` 35.9 s. `bench check` beside the other process, the two
  builds alternately, twice each: the budgets alike (the descent onto a people's world 79.2 and 78.3 ms before, 94.6 and 77.6 after;
  the forest at 2x 12.6 and 14.2, 13.4 and 13.7; the site's tiles by the capital ready 0.58 s after the cursor was set before, 0.29-0.35
  after); the second run after passed every budget.
