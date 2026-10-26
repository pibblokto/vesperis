# src/peoples: the peoples that lived on some worlds

What a dead people was and left: its age, history and plan, its towns, works, buildings and ruins, its records and their
grammar, its music, instruments, charts and signals. Generators like the worlds' (the draws, salts and `GEN_VERSION` rules of
`src/galaxy/CLAUDE.md` hold here too); they read `src/galaxy` and `src/core` alone (`make check`). Sizes are .h + .cpp in KB;
read the big files by range after a `grep -n`.

## Files
- `peoples` (44): a people's age, eras, collapse and profile: `peopleAgeOf`, `peopleProfileOf`
- `history` (29): eras, places, persons, events: `historyOf`, `historyOfWorld`; `buildHistory` runs the builder's phases. Its
  parts: `history_tables.h` (37: the works, inventions, trades, causes, the everyday's words), `history_names` (41: the names,
  `historyEventLine`), `history_builder.h` (34: `Builder`), `history_setup` (42: eras, places, the other people, `jointOf`),
  `history_events` (41: a year's events, the families, the collapse), `history_years` (34: the persons, the year loop)
- `plan` (66): territory, ranked places and ways on a coarse grid: `worldPlanOf`, `worldPlanPrefetch`
- `towns` (47): streets, squares and fields by people and age: `townStreets`, `townFields`
- `works` (26): what was built outside the towns, kept (the world's, the land's cells): `worldWorksOf`, `landWorkOf`. Its parts
  share `works_internal.h` (13: the builder `WB`, the land's grid): `works_sites` (73: the world's works, a finder a kind),
  `works_land` (29: B-418's, a cell's each), `works_pieces` (42: `workSpec`, `workLine`)
- `buildings` (49): decay, the kinds' ages, the dispatch `buildingForms`, the rooms (`buildingRoom`), the palette and ways. Its
  parts share `buildings_internal.h` (26: `Gen`, the walls', rings' and towers' specs, the helpers): `buildings_frame` (43:
  walls, roofs, towers), `buildings_dress` (51: R-412's dress, the street furniture), `buildings_medieval` (53),
  `buildings_ages` (72: the prehistoric to the far future), `buildings_works` (58: the genome's, the harbour's, T-03's kinds)
- `ruins` (61): settlements per 2 km cell, `Culture`, the monuments, the world's plan kept a thread: `ruinOfCell`, `cultureOf`,
  `peopleGen`. Its parts share `ruins_layout.h` (39: `Layout`, `TownHist`): `ruins_settlement` (35: `layoutSettlement`, the
  features), `ruins_towns` (36: `layoutTown`, the street furniture)
- `forms` (83): geometry primitives, broken states, the silhouette: `formMesh`, `formProfile`, `formTop`; `forms_colliders`
  (36: `formColliders`, `formSupportAt`) shares `forms_internal.h` (2) with it
- `genome` (25): a people's 51 architecture variables: `genomeOf`, `genomeDistance`, `techniqueOf`
- `roads` (24), `graves` (12): roads between settlements (`roadsOfCell`, `roadsNear`); burial grounds (`gravesOf`)
- `records` (69): where each record lies, its year and medium: `recordsOf`, `recordSitesOf`
- `shards` (56): the lore's functions, the calendar, shard draws, the text cache; `GRAMMAR_FILES` is `grammar/files.inc`:
  `loreOf`, `shardTextsOf`. `lore.h` (4): `Lore` and `loreSeed`, for the headers that only hold or pass a lore
- `genres` (87): the records' model; writes a people's texts: `writeRecordTexts`. `genres_schema` (26): the schema (in its .h
  the facts, entities, fields, directives and the history's keys; `schemaOf`, `recordSchemaKeys`), the world's facts (`recordWorldFacts`)
- `grammar` (62): the `.vg` engine's expansion and English realiser (`vgRecord`); its parts share `grammar_internal.h` (4):
  `grammar_parse` (31: `vgParse`, `vgLink`), `grammar_words` (22: the words' forms, `vgCheck`) (`grammar/`: its note)
- `tongue` (40), `voice` (21): a people's naming language (`tongueName`, `tongueWord`); its voice and speech (`voiceOf`,
  `speechOf`)
- `legends` (13): the history as the records read so far tell it: `toldHistoryOf`, `legendLine`
- `music` (60), `charts` (61): a tradition and its pieces (`traditionOf`, `pieceOf`; each age's texture in `music_ages`, 50,
  through `music_internal.h`'s builder `Bld`, 12); star-chart records (`chartOf`)
- `instruments` (76) + `instrument_plans.cpp` (135; declared in instruments.h): `instrumentsOf`, `instrumentPartsOf`
- `signals` (57), `drones` (11): the radar's transmitters and programmes (`signalsNear`, `programmeOf`); the drones' hauls

## Rules and traps
- No people shapes the planet function: of the worlds only `landmarks.cpp` and `planet_bodygen.cpp` read the peoples, and a
  people's seed never reaches the ground.
- State stays in its file: the thread_local caches (tongue.cpp:403; ruins.cpp:359, `planFor`; shards.cpp:645). The plan's and the
  texts' threads are joined at exit by `g_planJoin` (plan.cpp) and `textJobsJoin` (shards.cpp), in link order's reverse.
- A split module's parts share its internal header (`ruins_layout.h`, `history_builder.h`, `genres_schema.h`): helpers there
  `inline`, the rest declared and defined once; a part's own stay in its anonymous namespace. Inlined calls that now cross
  parts (`ageExpressed`, `buildingForms`'s generators, the history's phases, `pieceOf`'s builders) were timed: a split that
  moves one is timed (`bench`, or a scratch timer on the build's objects).
- Keys: a record is `<body>/S<people*1000+i>` (records.h:54). Instrument plans hold references into their parts: never past
  `reserve(1024)`.
- Generated music and speech are judged printed: `music` as notation at the subdivision's resolution, `voice` word by word with
  spans and sounds (the spoken rate is about 0.8 of the nominal). A tongue's words are two letters at least.
- The grammar's files move whole and no alternative's line is rewrapped (its text keys it); the harness walks the shelves of
  `src/peoples/grammar` and fails when a listed file is missing (`grammar where <rule>` finds a rule).

More, with the numbers: `docs/reference/lessons.md`, "Generation: the worlds, and the peoples".
