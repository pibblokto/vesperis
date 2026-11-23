# src/surface: on the ground

Over 50 KB (read by range): surface_view, flora, surface_terrain, site, surface_sky.

## Files
- `surface_view.h`: `SurfaceView`, data first (never reordered), then functions under a `// ----` banner per file, the harness's last.
  `surface_types.h`: its types (`Player`, `Buggy`, `Drone`, `SurfaceEnvironment`, the creatures, `Ruin`). `surface_internal.h`: what the
  surface files share (`SurfaceLook`, `lookFor`)
- `surface_view.cpp`: `init`, `update`, `render`, `reanchor`, the streaming (the ahead worker, `prefetch`), the scanner
- `surface_env` (`computeEnvironment`: light, weather, fog, the ground's songs), `surface_places` (cells and marks of settlements, works
  and towns, nearby records and graves, `drawRuins`), `surface_walk` (`updateWalking`, the colliders, `forRocks`, `drawObjects`, the
  capsule), `surface_palette` (`lookFor`, `setupPalette`, the lamps; B-456: the made colours and the hides dealt from one pool of the
  banks a world leaves free; B-457: each line's own, `growthBankOf`, `plantsBankOf`, read by the vertex's `mline`)
- `surface_terrain` (`vertexOf`, the material tiles, the rings, reflections, `heightNear`), `surface_sky` (the galaxy's band, `drawSky`,
  meteors, `drawVeil`; B-481: a deck world's overcast lit by a cloud line's glow at night), `surface_effects` (vents, geysers, spores,
  `drawWeather`)
- `site` (`SurfaceSite`, `TerrainCache`: the sun, `groundHeight`, `sampleAt`, nearby roads and marks), `flora` (`forTrees`, `forLogs`,
  `drawFlora`; J-09: a covered forest's trees wrapped; J-15: a carpet's grass, `greenedAt`: tufts and darker shrubs in its line's growth
  bank, B-482, no trees), `overgrowth` (J-09: an overgrowth's structures, `forGrowths`, `growthFeet`,
  `growthColliders`, `drawGrowth`; J-10: its sounds, `updateGrowthSongs`), `overgrowth_ruins` (J-10: a ruin's cover, `overgrowCell`,
  `stoneRotOf`), `creatures` (`planLife`, `updateLife`, `drawLife`; J-07: the ways' states, `swimmersNear`, the
  record `lifeSaw`; J-08: the fates on the herds, `cellMembers`, `cellRemains`, `remainsStageOf`, kills and feeding, remains drawn with
  stains and flies; B-467 to B-470: the shallows' stretches `shallowsPathOf`, a climber's side `climbSide` on the trunk as drawn
  (`trunkOf`, flora.cpp), a flock's and a shoal's fates, the landing's clock `lifeClock` and its kept dead `deadKept`, `restageRemains`,
  `lifeAnchored`; B-489 to B-492: the guide's dead laid in `deadIn`, a flock's members `flockMembers` and its falls `flockFalls`,
  `keepDead`, a climber's `climbPose` and `climbTreeNear`),
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
  `madeFateAt`; a world nobody seeded has none, and its herds are drawn as before, old bones the one thing more. B-470: their stage reads
  the landing's clock (`lifeClock`, run in `update` before the step's cap: the guide's clock runs in real time). A kill or a fall is
  kept on the body (`deadKept`; B-489: the game copies it to the guide and lays the world's in again at a landing and a load, `deadIn`):
  a cell planned again skips its seed and its remains come back from it; a new anchor drops `remains`
  (`lifeAnchored`), whose cells' lay them again.
- B-490: a flock's members are placed by `flockMembers` alone (the frame's and the update's), from the flock's own draws in their order:
  a new member's behaviour takes its randomness from the member's hash, never a draw of the flock's `Rng`.
- J-09: an overgrowth's structures stand on a lattice fixed to the world (KI-476: `growthCellOf`, a cube map's 16 m cells placed once a
  landing in its metres, emptied at `init`, on a re-anchor and with the layer's key); `forGrowths` gives a local cell those whose
  middles lie in it, of the world cells whose four corners its cover holds (B-484: `growthCornerHolds`, read on the world; B-483: across
  a cube's edge the face beyond's), so a structure can reach over its cell's edge: `forTrees` tests the cells round a tree's.
  Their feet (`growthFeet`) are the colliders' and the drawing's. Its bank (`growthBank`) is lent. Every reader is on the main thread.
- J-10: a ruin cell's cover (`climb`, each piece's by `pieceClimb`, its pieces after each building's at level 0) and a stone rotter's
  crumbling are made with the cell from `ruinsMade`, the world now the game hands on (`setMadeNow`), a changed key grows and lays them
  out again in `update` (`regrowRuins`); B-485: after that the ground takes the world now where `madeGroundKey` moved (`adoptMadeNow`:
  `site.made` and `site.gen.made`; the rings and the structures' corners on a worker, `startResample`, taken in 45 frames on by
  `takeResample`): `joinAhead` stops that worker too, so anything that changes what `sampleAt` reads (the roads, the streets, the works,
  the layer) calls it first, and nothing may keep a pointer into `site.gen.made` across `update`, and the
  harness's `testRegrowRuins` takes the layer it set. Its pieces never collide (`formColliders` skips `FORM_PART_GROWTH`). A torn
  membrane is kept by key (`tornMembranes`, emptied at `init` alone) and marked `Form::broken`.
- B-488: a ruin's building takes its own line (`RuinCell::lines`); `drawFormItems` makes the cover's raster parameters again for each other
  growing line from `RP_LINE0` (`lineBlock`), and a face across the cover's edge carries its second bank's offset (`FormPolyOut::s2`).
  B-487: a vertex's tile is `tileOf` (a crust line's crust in `madeCrustTiles`, made by the warm).
- J-11: a dead world's ground is read through `deadCoverOf(site.gen)` and `deadGroundOf` (galaxy/planetmap.h), never the kind's row's
  `cover`: a made hothouse and a made snowball lay one too (the trees, logs, undergrowth, bark banks, towns' frost, reservoirs). A made
  hothouse never rains on the ground (`computeEnvironment`).
- J-07: what is seen is what `drawLife` drew whole (`lifeSaw`): a run that only updates sees nothing. A way's own state ends by its own
  timer or event, never by the generic choice. Find an animal again by its seed: herds come and go and move the others.
- A new vehicle: the cookbook's "Add a vehicle".

More, with the numbers: `docs/reference/lessons.md`, "The surface".
