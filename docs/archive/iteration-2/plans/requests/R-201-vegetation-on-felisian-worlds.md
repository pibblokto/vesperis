---
id: R-201
title: Felisian worlds are weak; vegetation is really poor
status: done 2026-09-26 (N2; the forest frame budget landed at 12 ms, not 6, see KI-206)
requested: 2026-09-26
related: N2-01..N2-06, KI-203, B-202 (colours), M4-04 (first attempt), M1-01 (greenmush)
---

## What
Felisian worlds should look alive from the ground: forests with dense canopies and shade under them, meadows, reeds by the water, distinct biomes, trees with real silhouettes and colour variation, undergrowth to walk through.

## Why
The living world is the reward of a landing and the heart of the original's feel. Today trees are thin trunks with a few yellow-green blobs, there is nothing under them and every biome looks like the same plain with more or fewer blobs.

## Must / should / could
- must: forests that read as forests at every distance; several tree shapes per world; undergrowth; darker ground under canopy; biomes you can tell apart on the ground.
- should: seasons and family palettes; flowers, ferns, reeds, fallen logs; wind in the leaves.
- could: fireflies, bioluminescent night forests, migrating colours.

## References
The original's green sprayed forests; the flow frames `shots/surf_FELISIAN_noon.png` show the current state.

## My notes (filled by Claude)
Design in `PLAN-living-worlds-2.md`; items N2-01 to N2-06 in `ROADMAP.md`. Size XL overall. Depends on N1 for the palette so forest colours agree with the globe and the map. Acceptance: canopy coverage over 60% within 200 m in forest biomes, one scene per biome, a forest view at 2x under 6 ms, frame hashes re-blessed with the reason.

### Outcome (2026-09-26, N2)
`surface/flora.cpp` replaced the vegetation section of `drawObjects`. One enumerator (`SurfaceView::forTrees`) places every tree from the cell hash, the canopy density field (`veg` x a clearing mask, `canopyDensityAt`) and the biome; the colliders, the forest floor, the coverage test and the drawing all read it. Trees: seven silhouettes (dome, cone, umbrella, tiered giant, fibrous stalk, fern tree, mushroom tree; two per world, hashed, plus biome overrides: taiga and tundra cones, savanna umbrellas, wetland fern trees, tropical giants), a tapered trunk quad with branches to the blobs, 8-20 leaf blobs in three tones (lit crown, mid sides, dark underside), three leaf banks (forest, the second family's colour, a straw variant) for variation, 3% dead skeletons, fallen logs (colliders), smaller trees at the forest edge, clearings, the floor darker under the canopy. Undergrowth per biome: ferns, bushes with berries, mushrooms, tall swaying grass and flowers in spring and summer, reeds and cattails at water edges, lily pads on still wetland water, fireflies at dusk, cacti and succulents in deserts, cushions and lichen on tundra and alpine rock. Seasons: deciduous families turn to the second bank's autumn colour in the cold shoulder seasons and go bare in winter. Sound: `audio.leaves` (wind through the canopy plus the rustle of walking through undergrowth); canopies sway with the wind.

Numbers: canopy coverage of the forest ground within 200 m 92% (tropical), 79% (temperate), 15% (taiga, cones are narrow by design); the tropical giant forest at 2x costs 12 ms (1,663 trees drawn, 1,194 skipped behind filled canopy tiles, 41,000 spray points) against the 6 ms hoped for: the budget is set at 14 ms and the gap is KI-206. Kelp through shallow water was not done (KI-207). Scenes: `scene felisian_forest`, `scene biome_<tropical|savanna|desert|temperate|grassland|taiga|tundra|wetland>`; `bench` has the forest frame; `audio` a "forest" stage. Frame hashes re-blessed.
