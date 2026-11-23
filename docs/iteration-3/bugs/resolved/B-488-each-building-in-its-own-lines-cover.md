# B-488: each building in its own line's cover

status: resolved 2026-11-23 (J-16)
severity: S4
area: surface (`surface/overgrowth_ruins.cpp`: `overgrowCell`; `forms_draw.cpp`: `gatherSettlement`, `drawFormItems`)

## Report

KI-501, found in B-457 (2026-11-22): "A settlement's cover is the line's whose cover lies at its middle, one a settlement, and its
glowing patches at night follow the latest growing line's light." Listed for J-16 on the user's word (2026-11-23): "after that there
should be a J-16 that will basically handle all K-xxx that were created".

## Cause

`overgrowCell` read the line whose cover lies at a settlement's middle and gave it to the whole settlement (`RuinCell::growthLine`). Where
two lines' covers met in a town, every building took the middle's colour. The forms' cover was drawn with one set of raster parameters a
frame: the latest growing line's tile and its night light, so a line that glows lit nothing on its buildings when a later line did not.

## Fix

Each building takes the line whose cover lies at its own place (`RuinCell::lines`): its colour and bank, its tile, the grain of its
nodules and drapes, and its light at night. The forms' raster parameters for the cover are made again for each other growing line
(from `RP_LINE0`, the latest's where they were), and a building's pieces and faces under the cover draw with its own line's. Its far
silhouette takes its line's too.

## Verification (2026-11-23)

- The unit's `seeding` group, new check "each building of a settlement in its own line's cover": the scene's town placed beside it at
  night, a glowing flesh over it fourteen steps on and a fungus that does not glow, the latest line, over part of it. 83 covered buildings
  take the flesh and 7 the fungus, all 90 the line whose cover lies at their own place. In the frame, 5,810 faces of the flesh's buildings
  stand wholly under its cover, all 5,810 lit in its veins at night, and 375 of the fungus's, none lit. By the old code all 90 took the
  flesh, the line at the middle (83 of them rightly), and every face in view was drawn with the fungus's parameters: none of 5,490 lit.
