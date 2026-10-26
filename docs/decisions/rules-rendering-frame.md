# Rules: rendering and the frame: order, threads, time

The rules in force for the frame's draw order, threads, pool, time and exact arithmetic, a line each with the entries behind them
(`log/`, by release). A milestone that sets or changes one edits its line here; a replaced one moves to `superseded.md`
(`docs/CLAUDE.md`). The pixel, the banks, the light and the views: `rules-rendering.md`.

## Order and depth
- The star mask dims a star by its pixel's shade in any bank: the band and the aurora come after the stars, the aurora after the bodies [G-03], [B-403]
- A black hole's lens bends only what the space pass drew before the sun (band, stars, belts); a stripped tail follows its globe, depth-tested [S-06], [B-427]
- Sky overlays (chart figures) skip pixels with depth (`Framebuffer::invz`): never over the cabin, a globe or the ground; HUD markers over all [C-10]
- Depth is in metres (sky bodies over the ground never write kilometres); `toView` curves everything drawn on the surface [B-317], [O1], [B-302]
- Trees draw near to far and a band's canopy grid hides what its canopies cover: always `floraBands()` bands (`parallelBands`) [N2-04], [B-423], [F-06]
- Each terrain band draws `drawFloor`, its rings, then `drawReflectionBand`; `gatherReflections` runs serially after the bin (it fills caches) [F-05]

## The same pixels at any thread count
- Pictures are the same at any thread or core count: nothing drawn follows the bands' layout or who draws what [M7-04], [B-310], [A-02], [B-423], [F-06], [F-11]
- A pooled pass is row-pure or band-pinned, its items reading pure or read-only state into their own slots; rows split, never primitives [B-310], [F-03], [F-05]
- A band pass walks every piece in the serial order, its fills clipped to its rows (`setRasterBand`, reset after it) [B-310], [F-04], [F-05]
- What bands share (vertex caches, tiles, `CabinFrame`) is made before them and only read; no band reads another's rows; `glowDisc` after [B-310], [F-05]
- `binTerrainLOD` bins rows, not bands, by the draw's own helpers (`cellShoreSplit`, `polygonRows`, ...), so it never under-lists [F-04], [F-05]
- The camera feeds' noise is one xorshift32 walk along the rows: pooled rows start from `xorshiftRowStarts` [F-05]
- The telescope's plate is dealt 64 blocks at a time, the same blocks in the same frames at any threads; `plateFill` 0 fills it whole [F-06], [B-424]

## The pool
- Only the main thread dispatches (`pool::contended` stays 0); a dispatch inside a job, or from another thread while the pool is held, runs alone [F-05], [F-06]
- The pool deals chunks to whichever thread is free, never an even split; a dispatch ends with its work, no late worker holding it [X-02], [F-06]
- `POOL_MAX_THREADS` is eight, two cores left for the game's threads (PNG writer, drainage, ahead worker, synth); workers sleep, no spin [F-06]

## The frame's time
- Progressive work a frame shows runs in shares inside frames (the flight band's keyframes, the far scan), never on a thread [G-07], [C-15]
- A page that hides the scene in a flight keeps the band's keyframes building (`Game::keepFlightBand`); the opaque galaxy page draws no scene [G-05], [G-07]
- The platform paces desktop frames, not raylib (`SetTargetFPS(0)`): `floor(hz / 60 + 0.2)` refreshes apart, each swap held to 3/4 refresh before [F-08]
- An early-out skips only work that would write nothing, with a slack far over the rounding, proved bit for bit (`coneMissesFrame`) [F-02], [F-05], [F-07]
- Hot loops read their band's own locals, never a closure's captures; hot lookups stay inline (`PlanetMap`, `TerrainCache::at`) [F-05], [F-07]
- Kind tags and `matBank` are read once before a pixel loop; per-kind passes only on their kinds; no per-cell `std::function` past its buffer [P-00], [P-02]
- A star class's effect costs only its own frames: its pass skips pixels outside its strip [S-04], [S-05]
- One path per hot loop: sheen a flag on `BLEND_DARKEN` (hull faces only), a lit window one flag, lit and sunless globes one code [B-411], [D-04], [D-02]

## Exact arithmetic and caches
- Kept to the bit: `blendRectRGB`'s signed truncating `(c - p) * a / 255`, the rasteriser's two divisions a row (an ulp moves pixels) [F-05], [F-10]
- Kept to the bit too: `wrap2pi` (`fmod`'s bits), libm's `atan2` and `asin` in globe and band, `buildDirLUT`'s 1.2% FOV rule [F-07], [F-10], [F-02]
- The mush swaps `Framebuffer::back` in (no pointer to the pixels survives a pass), exact to kernels of 38; `rasterPolygon` keeps 15 vertices [F-07]
- `smoothstep` with equal edges is a step, never a NaN: any caller may pass equal edges [B-409]
- Globe and plate wrap their cloud drifts alike once a frame (`cloudDriftOf`): within a turn of each other [F-10]
- `BackdropKey` holds every input of the kept backdrop (camera, focal length and centre, size, `bandGen`); both band map writers move `bandGen` [F-07]
- `SpaceRenderer`'s caches are keyed by the bodies' seeds: a recording swaps another star's `StarSystem` in for one call [X-04]
- `sampleGalaxyBand` inverts `bandRowLat`, takes the map's size as a constant and wraps the longitude by a mask: change them together [G-03], [F-07]

[M7-04]: log/0-1.0.0.md
[N2-04]: log/0-1.0.0.md
[O1]: log/0-1.0.0.md
[B-302]: log/0-1.0.0.md
[B-310]: log/0-1.0.0.md
[B-317]: log/0-1.0.0.md
[B-403]: log/1-1.1.0.md
[G-03]: log/1-1.1.0.md
[S-06]: log/1-1.1.0.md
[C-10]: log/1-1.1.0.md
[S-04]: log/1-1.1.0.md
[S-05]: log/1-1.1.0.md
[X-02]: log/2-1.2.0.md
[X-04]: log/2-1.2.0.md
[B-409]: log/2-1.2.0.md
[G-05]: log/3-1.3.0.md
[G-07]: log/3-1.3.0.md
[C-15]: log/3-1.3.0.md
[A-02]: log/3-1.3.0.md
[B-411]: log/3-1.3.0.md
[B-423]: log/4-1.3.1.md
[F-02]: log/4-1.3.1.md
[F-03]: log/4-1.3.1.md
[F-04]: log/4-1.3.1.md
[F-05]: log/4-1.3.1.md
[F-06]: log/4-1.3.1.md
[F-07]: log/4-1.3.1.md
[F-08]: log/4-1.3.1.md
[F-10]: log/4-1.3.1.md
[F-11]: log/4-1.3.1.md
[B-424]: log/4-1.3.1.md
[B-427]: log/5-1.3.2.md
[P-00]: log/6-1.4.0.md
[P-02]: log/6-1.4.0.md
[D-02]: log/6-1.4.0.md
[D-04]: log/6-1.4.0.md
