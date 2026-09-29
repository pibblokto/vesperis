# B-402: the sector grid does not scale with the zoom

status: resolved 2026-09-29
severity: S3
area: surface, maps

## Report

Zooming the sector map out, the one-degree grid stays one degree: at 2048 km wide the lines are ten pixels apart and the map is "a yellow mush instead of a map".

## Fix

The grid's step is the first of 1, 2, 5, 10, 15, 30 degrees that keeps the lines at least 24 px apart at the current zoom. The one-degree cells carry the sectors' names when they are 46 px wide (128 and 512 km on a large world); the coarser lines carry their latitude along the left edge and their longitude along the top (`5N`, `20W`). `flow_sectormap_z1..z4.png` and `flow_sectormap_wide.png` show the levels.
