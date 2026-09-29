---
id: R-301
title: Sectors you can drive through, matching the map you chose them on
status: done 2026-09-27 (O2)
requested: 2026-09-27 (verbal)
related: B-302, O2
---

## What
"There should be some ways to seamlessly drive/go through sectors you deploy the capsule on, and so that it makes actual sense considering the map you are seeing when deploying a capsule."

## Why
The landing map called a 1 x 1.5 degree cell of the whole-planet map a "sector" (about 130 x 200 km) and drew it 1.4 px wide; nothing on the ground told you which sector you were in or when you left it; the sector map used its own colours. The surface was already one continuous sphere (the site re-anchors as you go), so crossing was seamless in fact but invisible.

## My notes (Claude)
* One grid: 1 x 1 degree cells named `LON:LAT` (longitude index from 180 W, latitude index from 90 S; `sectorIndexOf`, `sectorName` in `game/ui.h`), printed on the landing map, the surface HUD (line 2) and the sector map; crossing one on the ground says `ENTERING SECTOR 226:113 - GRASS, GRASSLAND` and writes a `SECTOR` entry in the log.
* The landing map's `Z` zooms into the cursor's sector: a window one sector tall and two sectors' worth of ground wide (the frame is 2:1), sampled from the planet function at the far ring's detail (64-512 m: what you will see from the site), the same colours as the whole-world map, the one-degree grid with names, a scale bar, cursor steps of a hundredth of a degree (about a kilometre; five with Shift); the window re-centres when the cursor nears its edge. `Z` again returns to the world.
* The sector map (`N`) takes the landing map's colouring (`Game::mapRampsFor`, `mapColor`) and a fourth level of 128 km with the grid and names, so the map you land by and the map you walk by agree.
* Sizes: on a 6000 km world a sector is 105 km square at the equator: a day's drive, a few hours on foot with the time warp; the rangefinder and the crosshair waypoint (O1) give the distances.
