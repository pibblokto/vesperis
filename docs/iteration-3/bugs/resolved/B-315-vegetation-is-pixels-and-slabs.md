---
id: B-315
title: Vegetation is ugly: leaves are pixels, logs are slabs, too little variety
severity: S2
status: resolved 2026-09-28
reported: 2026-09-28
---

## Where
State / screen: surface, felisian worlds
Star / body shown in the HUD: any
EPOC shown in the HUD: n/a
Save attached: no

## Steps
1. Land in a forest at the default 4x picture and look at the trees, their leaves and the fallen logs.

## Expected
Trees that read as trees: solid, shaded canopies with texture, trunks with bark, logs that are logs, and variety
from world to world.

## Actual
"Why are leaves just pixels? Lying logs are shit. I need far greater variety and for it to be far prettier" (the
user).

## Notes
Trees were trunks with sprays of hashed points for canopies (the mush melted them into blobs at 1x; at 4x they were
sparse pixel clouds) and logs were two flat quads. `surface/flora.cpp` rebuilt:

* **Leaf clusters.** A canopy is a set of clusters; each is a ragged disc facing the camera, a fan of 6-10 rim vertices
  with hashed radii round a centre, cut in the rasteriser by a leaf-edge noise (`RasterParams::cutout`: the vertex
  weight is 1.1 at the centre and 0.3 at the rim, so the rim breaks into tufts and a few holes open inside), shaded by
  which side of the canopy it sits on (the top and the sun's side bright, the far side and the underside dark), by the
  sun's side across the disc, and textured with a leaf tile at 12 texels a metre (bilinear close by, fading past
  60 m). It writes depth, so clusters, trunks and branches occlude one another. Far trees are one cluster on a trunk
  line, a speck below a pixel.
* **Trunks, branches, bark.** Tapered limbs facing the camera; the trunk and the near stalks carry a bark tile (8
  texels a metre across, 2 along: ridges, furrows and crack lines); branches run from the trunk to the first
  clusters; every silhouette can lean (the canopy centre sits off the foot). Bark and cut wood have their own palette
  banks (16 and 17, `setupPalette`): on the ground bank the dark stops take the sky's colour, and trunks came out black
  and branches grey-blue.
* **Ten silhouettes with a look per planet.** Dome, cone, umbrella, tiered giant, fibrous stalk, fern tree (its fronds
  are serrated leaf strips now), mushroom tree, and new: weeping (strands with small clusters hanging from the rim),
  candelabra (thick branches rising from the trunk's top, a ball of leaves on each), spire (small clusters stacked up
  a thin trunk to a bright tip). `TreeLook` (hashed per planet and silhouette) sets the canopy's proportions, the
  clusters' size and count, the trunk's thickness, the lean, the rim's raggedness, the leaf tone and the droop; the
  cold and the dry biomes choose per planet between cones and spires, umbrellas and candelabras, ferns and weeping
  trees.
* **Logs.** A six-sided trunk along its heading, sunk a third into the ground, bark-textured, each face lit by its own
  normal, a cut end toward the camera (pale wood in a dark bark ring), one or two broken stubs, a root plate on 30%,
  moss on the upper faces of 45% in wet and temperate biomes; thicker and longer than before.
* **Undergrowth.** Ferns are leaf strips within 28 m; bushes, succulents and cushions are clusters.

Cost: the rasteriser tests the depth before it reads a texel (a hidden pixel costs nothing more) and the polygon
clipper takes 16 vertices; the dense forest bench is 12.7 ms at 4x (12.9 before) and 7.7 at 2x. The sheet
`shots/tests/vegetation_sheet.png` shows forest, savanna, taiga, wetland, tropical and temperate sites at 4x.
