# src/surface: on the ground

## Files
- `surface_view.h/.cpp` (378 KB; read by range): `SurfaceView`: `update`, `render`, `reanchor`, the sky, the terrain rings,
  water, walking, the vehicles' cameras, the colliders
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
- Adding a vehicle (the buggy's pattern): a struct in surface_view.h, its own `surface/*.cpp`, the camera branch in `render`, the
  update dispatch, `reanchor` and `init`, the seat's HUD branch in `game/hud.cpp`, the keys and the audio in
  `game/game_states.cpp`, a save line, and `inVehicle()` wherever "in the buggy" meant "seated".

More, with the numbers: `docs/reference/lessons.md`, "The surface".
