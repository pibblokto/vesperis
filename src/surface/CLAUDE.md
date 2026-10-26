# src/surface: on the ground

## Files
- `surface_view.h` (54 KB): `SurfaceView`, its data first (never reordered), then its functions under a `// ----` banner for each
  file, the harness's last; `surface_types.h` (13 KB) its types (`Player`, `Buggy`, `Drone`, `SurfaceEnvironment`, `LocalFront`,
  the creatures, the trees, `Ruin`); `surface_internal.h` what its files share (`SurfaceLook`, `lookFor`)
- `surface_view.cpp` (44 KB): `init`, `update`, `render`, `reanchor`, the streaming (the ahead worker, `prefetch`), the scanner
- `surface_env.cpp` (22 KB): `computeEnvironment` (the light, the weather, the fog), the fog seas' floor, the ground's songs
- `surface_places.cpp` (20 KB): the settlements' and works' cells, the towns' marks, the plan's places; the records, instruments
  and graves near (`drawShards`); `drawRuins`
- `surface_walk.cpp` (51 KB): `updateWalking`, the colliders and supports, the boulders' enumerators (`forRocks`...) and
  `drawObjects`, the capsule, the blob shadows, the footprints
- `surface_palette.cpp` (25 KB): `lookFor`, `setupPalette`, the lamps (`setupLamps`, `lampAt`)
- `surface_terrain.cpp` (68 KB): `vertexOf`, the material tiles, the rings' warm, bin and draw, the reflections, the floor,
  `heightNear`, `rangeToGround`
- `surface_sky.cpp` (56 KB): the galaxy's band, `drawSky` (637 lines, whole), the shower meteors, `drawVeil`
- `surface_effects.cpp` (40 KB): the vents' jets, the geysers, spores, plumes and curtains, the lamps left lit, `drawWeather`
- `site.h/.cpp` (64 KB): `SurfaceSite`, `TerrainCache`: the sun, `groundHeight`, `sampleAt`, the roads and marks near the site
- `flora.cpp` (71 KB): `forTrees`/`forLogs`, `canopyDensityAt`, `drawFlora`, `drawFloraBand`
- `creatures.cpp` (37 KB): `planLife`, herds by habitat, `updateLife`, `drawLife`
- `bestiary.h/.cpp` (10 KB): `Species`, `Bestiary::make` (a world's fauna from its seed), `pickLand`
- `buggy.cpp` (31 KB): `updateBuggy`, `drawBuggy`, `cameraFeed` (the nose camera), `findOpenRun`
- `drone.cpp` (23 KB): `updateDrone`, `deployDrone`/`toggleDrone`, `drawDrone`, `droneCameraMount`
- `forms_draw.cpp` (26 KB): a settlement's forms: `gatherSettlement`, `pieceFootNear`, `drawFormItems`
- `textures.h/.cpp` (26 KB): `buildMesoTile`, `mesoVariant`, `buildMacroTile`, the leaf and bark tiles

## Rules and traps
- One planet function (`src/galaxy/planetmap`): the surface samples it and never keeps its own copy. A material never follows a
  faded value (the fade differs between rings, so a patch would pop and sweep along).
- The caches are in local metres, cleared at `init` and on `reanchor` (which joins the worker and clears the roads, marks, edits,
  ruin and work cells, supports and life). A per-world set the game owns (found shards) is refilled from the guide after each
  `init` (the landing map and the load).
- Threads: the caches and tiles warm first; band threads only read and never make a tile; the ahead worker samples into a
  staging buffer that the main thread copies in. Grow the roads, town marks or work edits only with the ahead worker joined
  (`joinAhead`). Nothing far off is computed on the main thread.
- Flora bands are pinned (`parallelBands`: 6 at 1x, 8 at 2x-4x): a band's coverage grid decides which far trees show.
- The colliders come from the enumerators that draw (`forRocks`, `forTrees`, `forLogs`), so the walls are what is drawn. A
  collider is a cylinder (`y0..y1`): anything new that blocks the explorer carries its span (`vesperis_test walls`).
- `groundHeight` is the ground drawn; the tide is in `waterNowAt`, and placed things read `waterAt`. `initFrame` makes a site that
  is sampled, never drawn. `applyDrainage`'s valley melt lifts exposed low ground: re-blend a dry bed after it.
- A call the compiler inlines stays in its file (the terrain's cell helpers, `materialLook` in `vertexOf`, `mesoFor` in the
  terrain's draw, `jetStrength`, `bandAt` in `drawSky`): a split crossing one is timed (`bench`).
- Adding a vehicle (the buggy's pattern): a struct in surface_types.h, its own `surface/*.cpp`, the camera branch in `render`, the
  update dispatch, `reanchor` and `init`, the seat's HUD branch in `game/hud.cpp`, the keys and the audio in
  `game/surface_play.cpp`, a save line, and `inVehicle()` wherever "in the buggy" meant "seated".

More, with the numbers: `docs/reference/lessons.md`, "The surface".
