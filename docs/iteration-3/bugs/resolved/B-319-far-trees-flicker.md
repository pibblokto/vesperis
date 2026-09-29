---
id: B-319
title: Far trees flicker all the time
severity: S3
status: resolved 2026-09-28
reported: 2026-09-28
---

## Where
State / screen: surface, living worlds, at the 4x default
Star / body shown in the HUD: any
EPOC shown in the HUD: n/a
Save attached: no

## Steps
1. Stand or walk on grassland or savanna with trees 100-400 m off.

## Expected
Far canopies hold still.

## Actual
"Vegitation is way better but further placed trees are flickering all the time" (the user).

## Notes
Three things, all per far canopy (`rPx` under 3.5 logical pixels, one cluster and a trunk line):

* the leaf-edge cutout (B-315) sampled its noise tile at about a texel a pixel on a disc a dozen pixels across, so a
  fraction of a pixel of camera movement tore a different set of pixels out of the disc every frame; the ragged edge
  now fades in between 3 and 8 logical pixels of radius (`cluster` in `drawFloraBand`);
* the leaf tile boiled the same way under every small disc; it fades out under six logical pixels;
* the feet stood on lod0's height while lod0's outer band (160-640 m) draws the ground morphed onto lod1 (B-313):
  trees, rocks and animals there sank and surfaced as the band swept over them. `SurfaceSite::groundHeight` follows
  lod0's morph now (`lod0R`, `lod0Band`, `camX/camZ` set per frame), as it followed the near ring's.

`stability <scene> 0.1 4` (a walking camera at 4x) reports the share of far-band pixels that jump per frame: 1.42% ->
1.23% on the herd site, 1.14% -> 1.00% on the grassland, and a new `skyline flips` count (pixels swapping between the
sky and anything else) for the next time. The rest of the far band's change is the ground texture's parallax and the
trunks' one-pixel lines, which a software rasteriser without anti-aliasing cannot hold still.
