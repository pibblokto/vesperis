# src/galaxy: everything drawn from the hashes

Generators: the same input always gives the same output, and the harness pins much of it. Two halves (E-09 moves the second to
`src/peoples/`): the worlds, and the peoples that lived on some of them. Of the worlds, only `landmarks.cpp` and `planetmap.cpp`
read the peoples' half. Sizes are .h + .cpp in KB; read the big files by range after a `grep -n`.

## The worlds
- `starfield` (52): the hashed star field, the galaxy's shape, star names: `starInSector`, `jumpTarget`
- `system` (93): orbits, spin, light, tides, `GEN_VERSION` (system.h:12): `StarSystem::generate`, `kindOf`
- `kinds` (204): `KINDS` (a row per `PlanetType`), materials, `TRAITS`: `kindHas`, `traitEligible`
- `planetmap` (352): the planet function: `BodyGen::make`, `sampleSurface`, `PlanetMap`
- `drainage` (64): rivers, valleys and lakes in cached tiles: `drainageAt`, `drainagePrefetch`
- `landmarks` (39): the scanner's sights per ~14 km cell: `sightsOfCell`, `sightsNear`, `ruinsNear`
- `describe.cpp` (8; declared in planetmap.h): `describeBody`, `describeStar`
- `almanac` (29): the next sky events at a place, the finders' registry (fronts' and showers' last): `almanacOf`
- `nights` (14): meteor showers, zodiacal light, planetshine: `meteorShowersOf`, `bodyShineAt`
- `fronts` (20): moving weather fronts: `frontsOf`, `frontWeatherAt`
- `probe` (61): giant atmospheres and the probe's fall: `giantAtmosphereOf`, `probeStageAt`
- `drifters` (24): sunless worlds in starless sectors: `drifterInSector`, `placeInSector`

## The peoples
- `peoples` (44): a people's age, eras, collapse and profile: `peopleAgeOf`, `peopleProfileOf`
- `history` (262): eras, places, persons, events: `historyOf`, `historyOfWorld`
- `plan` (66): territory, ranked places and ways on a coarse grid: `worldPlanOf`, `worldPlanPrefetch`
- `towns` (47): streets, squares and fields by people and age: `townStreets`, `townFields`
- `works` (181): what was built outside the towns, and its ground edits: `worldWorksOf`, `landWorkOf`
- `buildings` (345): the ages' buildings as forms, their decay and rooms: `buildingForms`, `buildingRoom`
- `ruins` (169): settlements per 2 km cell and their layout, `Culture`: `ruinOfCell`, `cultureOf`, `peopleGen`
- `forms` (123): geometry primitives and broken states: `formMesh`, `formColliders`, `formProfile`
- `genome` (25): a people's 51 architecture variables: `genomeOf`, `genomeDistance`, `techniqueOf`
- `roads` (24), `graves` (12): roads between settlements (`roadsOfCell`, `roadsNear`); burial grounds (`gravesOf`)
- `records` (69): where each record lies, its year and medium: `recordsOf`, `recordSitesOf`
- `shards` (71): lore, the calendar, shard draws, the text cache; includes the `.vg` files (`GRAMMAR_FILES`): `loreOf`, `shardTextsOf`
- `genres` (112): the records' model and schema; writes a people's texts: `writeRecordTexts`
- `grammar` (121): the `.vg` engine, the English realiser, the checks: `vgParse`, `vgRecord`, `vgCheck` (`grammar/`: its own note)
- `tongue` (40), `voice` (21): a people's naming language (`tongueName`, `tongueWord`); its voice and speech timing (`voiceOf`, `speechOf`)
- `legends` (12): the history as the records read so far tell it: `toldHistoryOf`, `legendLine`
- `music` (124), `charts` (62): a tradition and its pieces (`traditionOf`, `pieceOf`); star-chart records (`chartOf`)
- `instruments` (76) + `instrument_plans.cpp` (135; declared in instruments.h): `instrumentsOf`, `instrumentPartsOf`
- `signals` (56), `drones` (11): the radar's transmitters and programmes (`signalsNear`, `programmeOf`); the drones' hauls

## Rules and traps
- A change to what a star, system or surface looks like bumps `GEN_VERSION` once per series (saves and guides record it); 1.4.0
  is generation 16. Protected: a people's world and the home world never change kind or get a ground-moving trait, a people's
  seed never shapes terrain, Heilya II stays felisian, and the regress and bench worlds don't move.
- Never insert into a saved or drawn list. Kind and trait ids are saved keys and set the draw order (append before `PT_COUNT`,
  `TR_COUNT`); picks draw only below `*_DRAWN_COUNT`, and entries after it are given, not drawn.
- A new decision draws from a salt or stream of its own, after the old draws, so no older world, save or history moves. Pinned
  salts and the home star keep home and the regress worlds as drawn (signals.cpp:37, kinds.cpp:714).
- A share of a fixed catalogue is a seeded shuffle with a count from the seed, not a coin per item. A chain `next = hash(prev)`
  walks with a stride coprime with the catalogue's size. What the galaxy shows from afar (which stars transmit) is hashed per
  cell of sectors with a Poisson-like count, never per world. A new star class is a variety (a share of a family by
  `mix64(seed ^ c)`), never a new weight in the pick; a special world is a forced trait (its own hash, put first).
- What the view, the landmark finder and the harness all read lives here and reads the body, the system and `BodyGen`, never a
  site. A material never follows a faded value (planetmap.cpp:48).
- Frame paths call `*IfReady` (null or false while a worker builds); `*Of` builds or waits.
- `Rng` is passed by reference (a copy forks the stream). Moving code keeps each expression's shape (clang fuses multiply-adds
  per expression); about 120 expressions draw twice in one expression: leave them as written.
- State stays in its file: the thread_local guards and caches (drainage.cpp:53, 469, 542; planetmap.cpp:2063; tongue.cpp:403;
  ruins.cpp:1776; shards.cpp:623). `g_reaper` follows the tile cache (drainage.cpp:463) so that it joins the prefetches at exit.
  The almanac's finders are listed in almanac.cpp's `registry()`, whose order is the order of events at one moment (E-02:
  none adds itself at start-up any more, so link order no longer decides it).
- Keys: a record is `<body>/S<people*1000+i>` (records.h:20); saved places resolve through `placeInSector` (drifters.h:30).
  Instrument plans hold references into their parts: never past `reserve(1024)`.
- Generated music and speech are judged printed: `music` as notation at the subdivision's resolution, `voice` word by word with
  spans and sounds (the spoken rate is about 0.8 of the nominal). A tongue's words are two letters at least.

More, with the numbers: `docs/reference/lessons.md`, "Generation: the worlds, and the peoples".
