---
id: B-303
title: The sector map keeps the previous landing's marks on a new world or sector
severity: S3
status: resolved 2026-09-27
reported: 2026-09-27 (verbal: "when you deploy the capsule on a new planet or sector, marks on the local map don't reset, like where you were walking, etc.")
---

## Where
State / screen: the sector map (`N`) after landing somewhere else.

## Analysis (Claude)
`SurfaceView::init` reset the capsule, the player, the buggy (since B-205) and the footprints, but not the trail (600 points of where you walked), the waypoint or the eruptions; the trail and the waypoint of the last landing were drawn on the new one's map in the new frame's coordinates. The game's cached sector-map image was rebuilt only when you moved a tenth of the map width or changed the level, so a new landing whose local coordinates were near the old image's centre (every landing starts at the origin) showed the old world's terrain until you had walked far enough. The same two gaps existed at a re-anchor (every 50 km): the trail and the waypoint stayed in the old frame while the capsule and the buggy moved with it.

## Fix
* `SurfaceView::init` clears the trail, the waypoint, the eruptions (and their timer) and increments `siteEpoch`; `reanchor` converts the trail's points and the waypoint to unit vectors before the frame moves and back after, and increments `siteEpoch` too.
* `Game::renderSectorMap` keeps `sectorImgEpoch` and rebuilds the image when it differs; `load` resets it.

## Verification
`unit`: 40 trail points after 40 steps, a re-anchor at 51 km keeps the waypoint within 5 cm and a trail point within 50 cm of their places on the sphere, a new landing leaves the trail empty and no waypoint; `flow`: after the second landing `trail=1 waypoint=0` and `second_sectormap.png` shows only the new capsule and the explorer.
