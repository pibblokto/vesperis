# src/surface: on the ground

Over 50 KB (read by range): surface_view, flora, surface_terrain, site, surface_sky.

## Files
- `surface_view.h`: `SurfaceView`, data first (never reordered), then functions under a `// ----` banner per file, the harness's last.
  `surface_types.h`: its types (`Player`, `Buggy`, `Drone`, `SurfaceEnvironment`, the creatures, `Ruin`). `surface_internal.h`: what the
  surface files share (`SurfaceLook`, `lookFor`)
- `surface_view.cpp`: `init`, `update`, `render`, `reanchor`, the streaming (the ahead worker, `prefetch`), the scanner
- `surface_env` (`computeEnvironment`: light, weather, fog, the ground's songs), `surface_places` (cells and marks of settlements, works
  and towns, nearby records and graves, `drawRuins`), `surface_walk` (`updateWalking`, the colliders, `forRocks`, `drawObjects`, the
  capsule), `surface_palette` (`lookFor`, `setupPalette`, the lamps)
- `surface_terrain` (`vertexOf`, the material tiles, the rings, reflections, `heightNear`), `surface_sky` (the galaxy's band, `drawSky`,
  meteors, `drawVeil`), `surface_effects` (vents, geysers, spores, `drawWeather`)
- `site` (`SurfaceSite`, `TerrainCache`: the sun, `groundHeight`, `sampleAt`, nearby roads and marks), `flora` (`forTrees`, `forLogs`,
  `drawFlora`), `creatures` (`planLife`, `updateLife`, `drawLife`), `bestiary` (`Bestiary::make`), `buggy` (`updateBuggy`,
  `cameraFeed`), `drone` (`deployDrone`), `vehicle_shape.h` (R-427: the buggy's and the drone's stations, wheels, pods and skids, from
  which the ship's maintenance room builds its models), `forms_draw` (a settlement's forms), `textures` (`buildMesoTile`)
- `sampling` (Z-03: `readSample`, a place's microhabitat for a vial, `placeMeanC`. Films under the lamp's ultraviolet: `filmNicheOf`,
  `prepareFilmGlow`, `drawFilmGlow`. R-434: the activity vision, `prepareSight` and `drawSight`)

## Rules and traps
- One planet function (`src/galaxy/planetmap`): the surface samples it and keeps no copy. A material never depends on a faded value: the
  fade differs between rings, so a patch would pop.
- The caches are in local metres, cleared at `init` and on `reanchor` (which joins the worker first). A per-world set the game owns
  (found shards) is refilled from the guide after each `init`.
- Threads: the caches and tiles warm first. Band threads only read and never make a tile. The ahead worker samples into a staging buffer
  that the main thread copies in. Grow the roads, marks or work edits only with the worker joined (`joinAhead`). Nothing far off is
  computed on the main thread.
- Flora bands are pinned (`parallelBands`: 6 at 1x, 8 at 2x-4x): a band's coverage grid decides which far trees show.
- The colliders come from the enumerators that draw (`forRocks`, `forTrees`, `forLogs`): the walls are what is drawn. A collider is a
  cylinder (`y0..y1`): whatever blocks the explorer carries its span (`vesperis_test walls`).
- `groundHeight` is the ground drawn. The tide is in `waterNowAt`, and placed things read `waterAt`. `initFrame` makes a site that is
  sampled, never drawn. `applyDrainage`'s valley melt lifts exposed low ground: re-blend a dry bed after it.
- A call the compiler inlines stays in its file (`materialLook` in `vertexOf`, `mesoFor`, `jetStrength`, `bandAt` in `drawSky`). A split
  that crosses one is timed (`bench`).
- A microhabitat reads what the world fixes (the planet function, the features' finders, the walls), never what a landing places on its
  own grid (rocks, domes, trees, chimneys). So a sample of a square metre is the same from any landing (`life search`'s second read).
- The ultraviolet pass runs only while the ultraviolet is on, and its ramp (bank 15) is set only then. Off, the frame is the old one to
  the bit (the unit's `search` group). The pass reads the frame's grid made on the main thread, and its rows are pure.
- The activity vision reads its square metres on the pool with `readSample(x, z, false, false)`. Ruin cells fill as they are asked for,
  so the main thread finishes those reads with the walls afterwards (`wallsDone`). The vial's own read (`walls` true) is unchanged. The
  vision's metres are local metres (cleared at `init` and `reanchor`, and read again past a quarter metre of tide).
- A new vehicle: the cookbook's "Add a vehicle".

More, with the numbers: `docs/reference/lessons.md`, "The surface".
