# src/galaxy: the worlds, drawn from the hashes

Generators: the same input always gives the same output, and the harness pins much of it. The peoples that lived on some of
the worlds are `src/peoples/` (its own note): no world reads them but `landmarks.cpp` (the scanner hears ruins and works) and
`planet_bodygen.cpp` (a world's people), the two `make check` allows. Sizes are .h + .cpp in KB; read the big files by range
after a `grep -n`.

## Files
- `starfield` (52): the hashed star field, the galaxy's shape, star names: `starInSector`, `jumpTarget`
- `system` (93): orbits, spin, light, tides, `GEN_VERSION` (system.h:12): `StarSystem::generate`, `kindOf`
- `kinds` (204): `KINDS` (a row per `PlanetType`), materials, `TRAITS`: `kindHas`, `traitEligible`
- `planetmap` (35): the planet function's entry: `sampleSurface` (the kind's ground, then the solid grounds' common end),
  `PlanetMap`, `typeSky`, the clouds. Its parts share `planet_internal.h` (29: the sampling scale, the features' lattice, the
  types and functions the parts pass each other, `GroundCtx`): `planet_bodygen` (32): `BodyGen::make`, the materials' ramps
  and families, the climate; `planet_features` (40): the lattice's fields, the relief, the cliffs, and the finders
  (`planet_features.h`: vents, the great shield, the stripes, the storm cells, the ember fields); `planet_landforms` (59):
  `landformsAt`; `planet_felisian` (44): the living worlds' layer, `applyDrainage`; `planet_grounds`, `planet_grounds2`
  (56, 54): the kinds' ground functions
- `drainage` (64): rivers, valleys and lakes in cached tiles: `drainageAt`, `drainagePrefetch`
- `landmarks` (39): the scanner's sights per ~14 km cell: `sightsOfCell`, `sightsNear`, `ruinsNear`
- `describe.cpp` (8; declared in planetmap.h): `describeBody`, `describeStar`
- `almanac` (29): the next sky events at a place, the finders' registry (fronts' and showers' last): `almanacOf`
- `nights` (14): meteor showers, zodiacal light, planetshine: `meteorShowersOf`, `bodyShineAt`
- `fronts` (20): moving weather fronts: `frontsOf`, `frontWeatherAt`
- `probe` (61): giant atmospheres and the probe's fall: `giantAtmosphereOf`, `probeStageAt`
- `drifters` (24): sunless worlds in starless sectors: `drifterInSector`, `placeInSector`

## Rules and traps
- A change to what a star, system or surface looks like bumps `GEN_VERSION` once per series (saves and guides record it); 1.4.0
  is generation 16. Protected: a people's world and the home world never change kind or get a ground-moving trait, a people's
  seed never shapes terrain, Heilya II stays felisian, and the regress and bench worlds don't move.
- Never insert into a saved or drawn list. Kind and trait ids are saved keys and set the draw order (append before `PT_COUNT`,
  `TR_COUNT`); picks draw only below `*_DRAWN_COUNT`, and entries after it are given, not drawn.
- A new decision draws from a salt or stream of its own, after the old draws, so no older world, save or history moves. Pinned
  salts and the home star keep home and the regress worlds as drawn (peoples/signals.cpp:37, kinds.cpp:714).
- A share of a fixed catalogue is a seeded shuffle with a count from the seed, not a coin per item. A chain `next = hash(prev)`
  walks with a stride coprime with the catalogue's size. What the galaxy shows from afar (which stars transmit) is hashed per
  cell of sectors with a Poisson-like count, never per world. A new star class is a variety (a share of a family by
  `mix64(seed ^ c)`), never a new weight in the pick; a special world is a forced trait (its own hash, put first).
- What the view, the landmark finder and the harness all read lives here and reads the body, the system and `BodyGen`, never a
  site. A material never follows a faded value (planet_internal.h:30).
- Frame paths call `*IfReady` (null or false while a worker builds); `*Of` builds or waits.
- `Rng` is passed by reference (a copy forks the stream). Moving code keeps each expression's shape (clang fuses multiply-adds
  per expression); about 120 expressions draw twice in one expression: leave them as written.
- State stays in its file: the thread_local guards and caches (drainage.cpp:53, 469, 542; planet_felisian.cpp:228; the peoples'
  in their note). `g_reaper` follows the tile cache (drainage.cpp:463) so that it joins the prefetches at exit. The almanac's
  finders are listed in almanac.cpp's `registry()`, whose order is the order of events at one moment (E-02: none adds itself at
  start-up any more, so link order no longer decides it).
- Saved places resolve through `placeInSector` (drifters.h:30).

More, with the numbers: `docs/reference/lessons.md`, "Generation: the worlds, and the peoples".
