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
  `drawFlora`; J-09: a covered forest's trees wrapped), `overgrowth` (J-09: an overgrowth's structures, `forGrowths`, `growthFeet`,
  `growthColliders`, `drawGrowth`; J-10: its sounds, `updateGrowthSongs`), `overgrowth_ruins` (J-10: a ruin's cover, `overgrowCell`,
  `stoneRotOf`), `creatures` (`planLife`, `updateLife`, `drawLife`; J-07: the ways' states, `swimmersNear`, the
  record `lifeSaw`; J-08: the fates on the herds, `cellMembers`, `cellRemains`, `remainsStageOf`, kills and feeding, remains drawn with
  stains and flies),
  `bestiary` (`Bestiary::make`; J-06: each species from its own seed; J-07: its way, `words`, `speciesStride`, `speciesGait`; J-08:
  `AnimalFate`, `animalFateOf`, `animalChangesOf`, `changedBody`), `animal_body` (J-06: a species' body of parts, the sixteen plans'
  recipes, a world's style, `animalDistance`), `animal_mesh` (J-06: the rig built once a species, `poseAnimal`, `emitAnimal` by detail;
  J-08: the dead pose, stiff and laid down, girth, sag, a twisted spine, the fates drawn: skeletons, see-through, sores, tumours, stalks,
  two heads, a joined twin), `buggy` (`updateBuggy`,
  `cameraFeed`), `drone` (`deployDrone`), `vehicle_shape.h` (R-427: the buggy's and the drone's stations, wheels, pods and skids, from
  which the ship's maintenance room builds its models), `forms_draw` (a settlement's forms), `textures` (`buildMesoTile`)
- `sampling` (Z-03: `readSample`, a place's microhabitat for a vial, `placeMeanC` (J-03: galaxy's, which a seeding's place reads too).
  Films under the lamp's ultraviolet: `filmNicheOf`,
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
- The animals (J-06) are posed on the pool, each into a shape list of its own, and filled in bands in the jobs' order. A pose reads the
  ground only through the 3 by 3 heights the main thread read for it (`groundHeight` fills the cache on a miss).
- J-08: remains are a pure function of their cell: `cellRemains` reads the planet function (`sampleAt`), never the drawn ground
  (`groundHeight` and `waterAt` hold only what is loaded near the explorer). A species' fate reads the layer (`site.gen.made`) through
  `madeFateAt`; a world nobody seeded has none, and its herds are drawn as before, old bones the one thing more.
- J-09: an overgrowth's structures are a pure function of a 16 m cell whose four corners its cover holds (`forGrowths`). Their feet
  (`growthFeet`) are the colliders' and the drawing's. `forTrees` reads them so no tree stands in one. Its bank (`growthBank`) is lent.
- J-10: a ruin cell's cover (`climb`, its pieces after each building's at level 0) is made with the cell from the layer; the harness grows
  it again after it changes the layer (`testRegrowRuins`). Its pieces never collide (`formColliders` skips `FORM_PART_GROWTH`).
- J-11: a dead world's ground is read through `deadCoverOf(site.gen)` and `deadGroundOf` (galaxy/planetmap.h), never the kind's row's
  `cover`: a made hothouse and a made snowball lay one too (the trees, logs, undergrowth, bark banks, towns' frost, reservoirs). A made
  hothouse never rains on the ground (`computeEnvironment`).
- J-07: what is seen is what `drawLife` drew whole (`lifeSaw`): a run that only updates sees nothing. A way's own state ends by its own
  timer or event, never by the generic choice. Find an animal again by its seed: herds come and go and move the others.
- A new vehicle: the cookbook's "Add a vehicle".

More, with the numbers: `docs/reference/lessons.md`, "The surface".
