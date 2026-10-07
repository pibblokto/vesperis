# The surface picture: smooth far rendering, no artifacts

The user's ask after C-09 (2026-10-03), answered on paper then and scheduled for 1.2.0 as the V
series of `MILESTONES.md`: keep the look (a software rasteriser at 320x200 through palette
intensity ramps, Noctis-style) and fix the picture. Far ground should be the same thing as near
ground, nothing should pop or step, hills should not be banded facets, distance should read as
distance. Not a new renderer: every item is engine work inside `src/surface` and `src/core`, no
generation bump (the planet function, the saves and the pinned sites hold), the four frame hashes
of the regress re-blessed deliberately per milestone, the generation hashes untouched.

## 1. What makes it look meh (read from the frames)

1. **Far ground is a different thing from near ground.** The rings sample the planet function at
   their own detail (4, 16, 64, 512, 2048 m; `lodFade` keeps only the octaves four cells and longer,
   KI-310) and switch at their edges with a morph band (B-313, B-318), so the 512 m and 2048 m rings
   are flat, coarse and lit differently and the horizon reads as a backdrop. A range 10 km away is
   softer than it will be at 5 (KI-318); the fine landforms end 120 m out (KI-320); a lake the coarse
   ring cannot see morphs onto nothing (KI-323); 3-4% of the ground still differs in material between
   the far rings (KI-341).
2. **Popping and seams.** Cells appear as the rings sweep, trees and rocks pop in at their draw
   distances (trees 24 cells, bushes 320 m, the far forests as cover points), the shore steps where a
   water quad's corner crosses the sea level.
3. **Flat shading.** A vertex's normal comes from its four cache neighbours at the ring's own cell
   size, so a hill is banded facets whose banding changes with the ring; the ambient is a hemisphere
   term of `n.y` alone; the intensity ramp has 36-50 steps and the mush filter smears the bands rather
   than hiding them.
4. **No depth cues.** The fog is one exponential of the distance from the look table; the sky meets
   the ground at a line; no height in the fog, no haze band at the horizon, no scattering toward the
   sun; the flora, the water, the ruins and the sky bodies fog each by their own rule.
5. **Ground texture.** The meso tiles (`surface/textures.*`, 32 m periodic) fade by the ring and far
   ground is one tone per material: a plain is a flat colour from 2 km out.
6. **Water and sky.** The water is a shade with a glint and a grain; no foam at the shore, the
   reflections are a Fresnel tint; the clouds are a sky-dome field without parallax.

## 2. The fixes, as milestones

* **V-01 Continuous LOD.** One scheme of nested rings with full geomorphing everywhere (clipmap- or
  CDLOD-style: every vertex morphs toward the coarser ring's surface by its distance, not only in a
  band at the ring's edge), the same shading rule and the same material tiles at every distance,
  distance thinning detail only. The sweeps' popping and the shore steps go with it (the water edge
  morphs with the ground). Budget: the bench's surface rows (`surface 2x` 6.5 of 16 ms, `descent
  first` 40 of 80) stay within their budgets; the serial baseline (`VESPERIS_THREADS=1`) is the proof
  of the threading contract. The largest item; everything else sits on it.
* **V-02 Normals and shading.** Normals from the planet function's gradient (the material chain's
  `gradM` of B-404, the same in every ring) blended with the sampled surface's at the near rings,
  ambient by slope and the sky's brightness, a finer intensity ramp (the ramps' steps and the
  `shade = 50 light^0.6` curve revisited), the mush filter judged again against the finer ramp.
* **V-03 Aerial perspective.** Height-aware fog (thicker in the valleys, thinner on the ridges),
  per-pixel in the terrain pass and shared with the water, the flora, the objects and the sky bodies;
  a haze band at the horizon that the sky's bank carries; scattering toward the sun (a brighter,
  warmer haze on the sun's side); the look table's fog distances re-read as a density per world.
* **V-04 Flora and rock LOD.** Distance-faded instancing (a tree's clusters thin with distance before
  the cover points take over), far trees and rocks as billboards from a per-species sprite drawn
  once a site, forests that do not shimmer (the stability sheets' flora region the proof), no pop at
  24 cells.
* **V-05 Ground texture to the horizon.** The meso tiles alive at every ring at coarser octaves (a
  texel the ring's own fraction of the tile), the macro tone tile carrying variation at all scales, so
  a plain 10 km out is not one tone.
* **V-06 Water and sky.** Shoreline foam from the water's depth and the ground's slope, reflections
  of the sky's bank and the sun by the wave normal, a cloud layer with real parallax (the cloud field
  at a height, intersected by the sky ray) that the shadows of L-02 can later use.
* **V-07 The artifact audit.** Every site of `landforms sites`, the flow's landing and the stability
  sheets at 1x-4x, the capsule holes test, the seam checks: KI-310, 318, 320, 323 and 341 closed or
  re-scoped against the new scheme, new KIs for what remains.

## 3. Constraints

* No generation bump: nothing of V changes `sampleSurface` at a given detail. Where V-01 changes
  which detail a point is drawn at, the heights it draws are still the planet function's at that
  detail (the regress's generation hashes are the check).
* The threading contract (rings and flora drawn in parallel bands, samplers on the render threads).
* The palette pipeline: colour is a bank and a shade, never RGB; the globe, the maps and the ground
  share the material colours, so V-02 changes the ground's shade curve, not its colours.
* Budgets: `bench check` after every milestone, the serial baseline once per milestone.

## 4. Order

V-01, then V-02 and V-03 (cheap next to it and visible in every frame), V-04, V-05, V-06, V-07
last. Each is judged on the stability sheets, the regress frames (re-blessed) and the user's play.

## 5. Reading

`READING-terrain-and-lod.md`: links the user handed over on 2026-10-04 (quadtree LOD, dual contouring, surface nets, cube spheres, noise), to investigate before the direction is discussed.
