---
id: B-322
title: Water artefacts: bars from the water when the eye is low, stair shores and a line from the air
severity: S2
status: resolved 2026-09-28
reported: 2026-09-28
---

## Where
State / screen: surface (swimming), the ascent ("RETURNING TO THE STARDRIFTER")
Star / body shown in the HUD: IMAIBECK III-A 40.1N 31.6W (an ocean world with ice floes), also felisian rivers from the air
EPOC shown in the HUD: 6011:003.853
Save attached: no (the autosave `vesperis_save_auto.txt` of the session, star 160 5 125 body 5)

## Steps
1. Swim off an ice floe on an ocean world; lower the camera (crouch, or a dive and its return).
2. Recall the capsule and watch the ground during the ascent.

## Expected
Water that looks like water from any eye height; from the air, floes and rivers with their real outlines.

## Actual
"When I lower camera (I was in the water) there was this ugly artifact": the water turns into a sheet of sky colour with tall
dark bars standing in it, a thin strip of blue at the horizon. "Getting back to the stardrifter... water rendered weird, weird
lines": from the capsule the floes are stairs of cells with cliff faces at their edges and a bright straight line crosses the
ground.

## Notes
Five causes, all fixed (`vesperis_test spot <sx> <sy> <sz> <body> <lat> <lon> <t> swim|ascent|dump ...` reproduces a frame at
an exact place from a save):

* **The bars.** `drawReflections` mirrored each shore cell as a quad whose wet corners sat at the water level itself: a slab
  under the whole cell. From an eye a few centimetres over the water the slabs of the shore cells passed under the camera
  and, clipped at the near plane, stood in the water as tall dark bars; the water itself went the sky's colour because the
  Fresnel term reached 1 at grazing angles. Now only the part of a cell that stands above the water is mirrored (each
  triangle clipped against the level, then folded under it), the grazing angle is clamped at cos 0.04 (the waves keep the
  mirror short of grazing), the eye under the drawn water surface is seen through water whatever the body does
  (`viewUnderwater`: the palette tint and the 22 m fog follow the eye, not `player.underwater`), and crouching afloat no
  longer sinks the eye under the surface.
* **The cliffs.** An ocean world's floes were plates at +1 m over a sea floor at -150 m with nothing between: a 151 m wall in
  one cell, so the shore snapped to the vertices and a floe seen from the water was a wall. The floe zone now lies over a
  shelf sea (the floor at 6.5 m) and every plate rises through a submerged ice foot 150 m wide (GEN 9).
* **The stairs.** Every coarse ring draws its ground with `zbias` 0.999 (B-313, so the finer ring wins where they coincide)
  but drew its water at full depth: from a kilometre away a metre-high shore lost to the plane in every wet cell and the coast
  snapped to the grid (KI-321's stair). The water now carries its ring's bias. And the edge itself: a cell straddling a
  water's edge is cut along it (`shoreSplitTriangle`, the cut at the water level), the edge being the contour of the shore
  distance (`SurfaceSample::shore`, `TerrainVertex::shore`: a river's or a lake's outline, the height over the plane where
  there is none) rather than the place where the two triangles happen to cross the plane; the shore distance morphs with
  the height at the ring edges (`VtxCache::sh`, `coarseShoreAt`), the coarse surface the morph targets is the cut surface
  (`cacheHeightAt`), a drawn vertex is clamped to its side of the edge, and `groundHeight` reads the same pieces. The water
  sheet is drawn where a corner lies on the water's side; it used to be drawn wherever a corner lay within half a metre of the
  level, which reached over the low ground beyond a river's plain as a moat of cells.
* **The floodplain.** A river's plain stands over its plane out to 3.5 half-widths and a lake's to 1.6 radii (`bankFloor`), the
  channel lies under it: the fine relief of the outer plain used to dip under the plane and the plane showed through it in
  flat patches a hundred metres wide (the "lakes" beside every river from the air).
* **The line.** The capsule's beacon beam, drawn from the ground while the camera flew with the capsule. The capsule is not
  drawn during the descent and the ascent.

Also: the water's depth shade reads the drawn (morphed) ground, so the ring seams no longer show in the water; the inner rings
hang no skirts (their edges lie on the coarser surface since B-313, and a skirt peeked through as a thin bright line).

Verified: `spot` frames of the swim at eye heights 0.5, 0.1, -0.02 m and of the ascent at 600 and 1500 m; `capsule
felisian_river` (the river's edges natural at eight altitudes, the sawtooth gone); scenes `felisian_river`, `felisian_lake`,
`felisian_shore`; `unit` (the seams still 0.0000 m). KI-321 closed, KI-322 narrowed to the ring edge's fade.
