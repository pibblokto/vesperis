# src/peoples: the peoples that lived on some worlds

What a dead people was and left, generated as the worlds are (the rules on draws, salts and `GEN_VERSION` in `src/galaxy/CLAUDE.md` hold
here), reading only `src/galaxy` and `src/core`. Over 60 KB (read by range): instrument_plans, forms, genres, buildings_ages,
instruments, works_sites, records, charts, music, plan.

## Files
- A people: `peoples` (`peopleAgeOf`, `peopleProfileOf`), `genome` (`genomeOf`), `tongue` (`tongueName`), `voice` (`voiceOf`, `speechOf`)
- `history` (`historyOf`, and `buildHistory`, which runs the phases): `history_tables.h`, `history_names` (`historyEventLine`),
  `history_builder.h` (`Builder`), `history_setup`, `history_events`, `history_years`. `legends` (`toldHistoryOf`)
- The land: `plan` (`worldPlanOf`), `towns` (`townStreets`), `roads`, `graves`, `works` (`worldWorksOf`, `landWorkOf`) with
  `works_internal.h`, `works_sites`, `works_land`, `works_pieces`
- Buildings: `buildings` (`buildingForms`, `buildingRoom`) with `buildings_internal.h` (`Gen`), `buildings_frame`, `buildings_dress`,
  `buildings_medieval`, `buildings_ages`, `buildings_works`. `forms` (`formMesh`) with `forms_colliders` and `forms_internal.h`. `ruins`
  (`ruinOfCell`, `cultureOf`) with `ruins_layout.h`, `ruins_settlement`, `ruins_towns`
- The records: `records` (`recordsOf`), `shards` (`loreOf`, `shardTextsOf`, and `GRAMMAR_FILES`, which is `grammar/files.inc`),
  `lore.h`, `genres` (`writeRecordTexts`, and Z-06's `writeWatermark`) with `genres_schema`, `grammar` (the `.vg` engine: `vgRecord`)
  with `grammar_internal.h`, `grammar_parse`, `grammar_words` (`grammar/` has its own note)
- Z-06: `made` (the made organisms: `madeSpeciesOf` gives the species a late people made on its world, `madeWatermarkOf` a made species'
  watermark as a record). It is the only file here that reads `src/life` (tools/check.py's exception), and its header names none of it.
- Music and the air: `music` (`traditionOf`, `pieceOf`) with `music_ages` and `music_internal.h`, `charts` (`chartOf`), `instruments`
  (`instrumentsOf`) with `instrument_plans.cpp` (declared in instruments.h), `signals` (`signalsNear`, `programmeOf`), `drones` (the
  hauls)

## Rules and traps
- No people shapes the planet function: of the world code only `landmarks.cpp` and `planet_bodygen.cpp` read the peoples, and a people's
  seed never reaches the ground.
- Frames ask `*IfReady` (null or false while a worker builds). `*Of` builds or waits. State stays in its file (`thread_local`). The
  plan's and the texts' threads are joined at exit (`g_planJoin`, `textJobsJoin`), in reverse link order.
- A split module's parts share its internal header: helpers there are `inline`, the rest defined once. A part's own helpers stay in its
  anonymous namespace. A split that moves an inlined call across parts is timed (`bench`, or a scratch timer).
- A record's key is `<body>/S<people*1000+i>` (records.h's `SHARD_STRIDE`). Instrument plans hold references into their parts: never
  grow the parts past `reserve(1024)`.
- Music and speech are judged from printouts: `music` as notation, `voice` word by word (spoken at about 0.8 of the nominal rate). A
  tongue's words are two letters at least.
- The grammar's files move whole, and no alternative's line is ever rewrapped: its text is its key.
- A text written outside the catalogue (a chart's caption, Z-06's watermark) takes no memory and tells no event, so no people's records
  move with it. Its words go `@apart`.

More, with the numbers: `docs/reference/lessons.md`, "Generation: the worlds, and the peoples".
