# B-487: a world's own crust in its own tile

status: resolved 2026-11-23 (J-16)
severity: S4
area: surface (`surface/surface_terrain.cpp`: `growthTexture`, `madeCrustForm`, `tileOf`, `warmTerrainLOD`, `drawTerrainLOD`)

## Report

KI-493, found in B-460 (2026-11-22): "A world's own crust takes a crust line's form on that world: the ground keeps one tile a material a
landing." Listed for J-16 on the user's word (2026-11-23): "after that there should be a J-16 that will basically handle all K-xxx that
were created".

## Cause

The ground keeps one detail tile for each material a landing (`mesoTiles`). B-460 drew a crust line's crust in its form by building the
crust's tile in that form (`growthTexture(MAT_CRUST)`), so on a world that had a crust of its own, its own crust took the line's form
too: polygons or ripples where the world had rosettes.

## Fix

A crust line's crust has tiles of its own, one for each form it is drawn in (`madeCrustTiles`). A vertex of crust where a line's look
lies over a half (its made tint, so its cover is the line's) takes the form of that line's look (`madeCrustForm`); any other crust, the
world's own, keeps the world's tile (`tileOf`). The terrain's warm makes a form's tile the first time its disc meets it, so the band
threads only read the tiles. `growthTexture` is the overgrowth's alone again.

## Verification (2026-11-23)

- The unit's `seeding` group, check "a crust's forms and a lush forest": on a landing with a crust line of ripples, its crust is drawn in
  the ripples (0 apart from that form's tile) and the world's own crust beside it in its own tile (0 apart from the world's). By the old
  code the world's own crust took the ripples.
- The three forms' tiles are as B-460 made them (66,732, 78,279 and 83,799 apart); a world nobody seeded draws its crust as before.
