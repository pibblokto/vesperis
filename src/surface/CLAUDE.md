# src/surface: on the ground

Over 50 KB (read by range): surface_view, flora, surface_terrain, site, surface_sky.

## Files
- `surface_view.h`: `SurfaceView`, its data first (never reordered), then its functions under a `// ----` banner a file, the
  harness's last; `surface_types.h` its types (`Player`, `Buggy`, `Drone`, `SurfaceEnvironment`, the creatures, `Ruin`);
  `surface_internal.h` what its files share (`SurfaceLook`, `lookFor`)
- `surface_view.cpp`: `init`, `update`, `render`, `reanchor`, the streaming (the ahead worker, `prefetch`), the scanner
- `surface_env` (`computeEnvironment`: light, weather, fog; the ground's songs), `surface_places` (the settlements', works'
  and towns' cells and marks; records and graves near; `drawRuins`), `surface_walk` (`updateWalking`, the colliders,
  `forRocks`, `drawObjects`, the capsule), `surface_palette` (`lookFor`, `setupPalette`, the lamps)
- `surface_terrain` (`vertexOf`, the material tiles, the rings, reflections, `heightNear`), `surface_sky` (the galaxy's band,
  `drawSky`, meteors, `drawVeil`), `surface_effects` (vents, geysers, spores, `drawWeather`)
- `site` (`SurfaceSite`, `TerrainCache`: the sun, `groundHeight`, `sampleAt`, the roads and marks near), `flora` (`forTrees`,
  `forLogs`, `drawFlora`), `creatures` (`planLife`, `updateLife`, `drawLife`), `bestiary` (`Bestiary::make`), `buggy`
  (`updateBuggy`, `cameraFeed`), `drone` (`deployDrone`), `vehicle_shape.h` (R-427: the buggy's and the drone's stations, wheels,
  pods and skids, which the ship's maintenance room builds its models from), `forms_draw` (a settlement's forms), `textures`
  (`buildMesoTile`)
- `sampling` (Z-03: `readSample`, a place's microhabitat for a vial, `placeMeanC`; the films under the lamp's ultraviolet:
  `filmNicheOf`, `prepareFilmGlow`, `drawFilmGlow`; R-434: the activity vision, `prepareSight` and `drawSight`)

## Rules and traps
- One planet function (`src/galaxy/planetmap`): the surface samples it and keeps no copy. A material never follows a faded
  value: the fade differs between rings, so a patch would pop.
- The caches are in local metres, cleared at `init` and on `reanchor` (which joins the worker first). A per-world set the
  game owns (found shards) is refilled from the guide after each `init`.
- Threads: the caches and tiles warm first; band threads only read and never make a tile; the ahead worker samples into a
  staging buffer the main thread copies in. Grow the roads, marks or work edits only with it joined (`joinAhead`). Nothing
  far off is computed on the main thread.
- Flora bands are pinned (`parallelBands`: 6 at 1x, 8 at 2x-4x): a band's coverage grid decides which far trees show.
- The colliders come from the enumerators that draw (`forRocks`, `forTrees`, `forLogs`): the walls are what is drawn. A
  collider is a cylinder (`y0..y1`): whatever blocks the explorer carries its span (`vesperis_test walls`).
- `groundHeight` is the ground drawn; the tide is in `waterNowAt`, and placed things read `waterAt`. `initFrame` makes a site
  that is sampled, never drawn. `applyDrainage`'s valley melt lifts exposed low ground: re-blend a dry bed after it.
- A call the compiler inlines stays in its file (`materialLook` in `vertexOf`, `mesoFor`, `jetStrength`, `bandAt` in
  `drawSky`): a split that crosses one is timed (`bench`).
- A microhabitat reads what the world fixes (the planet function, the features' finders, the walls), never what the landing deals
  on its own grid (rocks, domes, trees, chimneys): a sample is its square metre's from any landing (`life search`'s second read).
- The ultraviolet's pass runs only while it is on, and its ramp (bank 15) is set only then: off, the frame is the old one to the
  bit (the unit's `search` group). It reads the frame's grid made on the main thread; its rows are pure.
- The activity vision reads its square metres on the pool with `readSample(x, z, false, false)`: the ruins' cells fill as they are
  asked for, so the walls are the main thread's after (`wallsDone`); the vial's own read (`walls` true) is unchanged. Its metres are
  the local metres' (cleared at `init` and `reanchor`, and read again past a quarter metre of tide).
- A new vehicle: the cookbook's "Add a vehicle".

More, with the numbers: `docs/reference/lessons.md`, "The surface".
