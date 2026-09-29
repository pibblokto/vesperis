# B-401: sights everywhere on the sector map

status: resolved 2026-09-29
severity: S2 (the map is unusable at the middle zooms)
area: surface, maps

## Report

The moment you land, the sector map (`N`) shows every landmark of the site: glyphs at every level and names at 8-128 km wide, dozens of them over the terrain. The user asked to handle it "even if removing sights on the map is required", and doubted that all of them belong on the map even if added as they are found.

## Cause

`renderSectorMap` drew `surf.landmarks` (the whole drawn disc plus 8 km, one landmark per 14 km cell) with the generated names; the landing zoom drew every landmark of the zoomed sector with the eight most prominent named.

## Fix

Both maps draw a landmark only once it is in the guide's `landmarksSeen` (logged within a kilometre of the explorer, or named); the sector map keeps the nearest 24; a name is drawn only when the explorer gave one (R-401). A fresh landing shows an empty map that fills as you walk. `flow_sectormap*.png` are the frames.
