# The harness's modes

Written by `vesperis_test help md` from `src/harness/main.cpp`'s `MODES`: edit the table, not this page. `vesperis_test <mode> [arguments]`; `scale=N` anywhere renders at N x; no mode runs `stars`.

## galaxy

| Mode | Arguments | What it does | File |
| --- | --- | --- | --- |
| `stars` |  | the star and body statistics round the start region, four systems in detail (no argument runs it) | modes_galaxy.cpp |
| `galaxy` |  | G-01: the density function in numbers, the home sectors, the sky from above the home star | modes_galaxy.cpp |
| `chart` |  | G-05: the chart's column against the field, its cost, and the star map's galaxy page at every level | modes_galaxy.cpp |
| `crossing` |  | G-07: two long flights from home through the cabin, ahead and aside | modes_galaxy.cpp |
| `home` |  | G-02: the new-game search's candidates, each parked at its first living world and timed | modes_galaxy.cpp |
| `survey` | `[n]` | M9-01: distributions over n bodies near the start (2,000): types, heat, gravity, moons, materials, the sky | modes_galaxy.cpp |

## space

| Mode | Arguments | What it does | File |
| --- | --- | --- | --- |
| `maps` |  | a planet map per type | modes_space.cpp |
| `space` |  | each sun class from its first orbit, the globes, rings and belts, the telescope's and the fronts' frames | modes_space.cpp |
| `moonsky` |  | the sky from giants' moons, an eclipse, ringed and substellar skies; a binary's ground | modes_space.cpp |
| `almanac` | `[<sx> <sy> <sz> <body> [latDeg lonDeg [t [ship]]]]` | W-01: a place's almanac and its cost (the unit's three test places by default) | modes_space.cpp |
| `showers` | `[<sx> <sy> <sz> <body>]` | W-04: the meteor showers of the systems near home, or a world's | modes_space.cpp |
| `seasons` |  | M5-06: the day at 82 N of a tilted felisian world at both solstices | modes_space.cpp |
| `probe` | `[<sx> <sy> <sz> <body>] [chute] [night] [wav] \| giants \| stability [scale]` | X-01: a giant's air and a probe's fall into it; X-03: the giants' characters; X-02: the decks held still | modes_probe.cpp |

## surface

| Mode | Arguments | What it does | File |
| --- | --- | --- | --- |
| `surface` | `[type]` | every landable type (or one) at noon, sunset, night and at 82 degrees | modes_surface.cpp |
| `scene` | `<name>\|list [scale] [warm]` | a scene of scenes_table.h, its numbers printed; `list` names them all | modes_surface.cpp |
| `capsule` | `[scene]` | B-318: a scene's ground from the descending capsule at eight heights, sky through the ground counted | modes_surface.cpp |
| `spot` | `<sx> <sy> <sz> <body> <latDeg> <lonDeg> <t> <swim\|ascent\|stand\|dump> [pitchDeg] [yawDeg] [alt]` | B-322: a frame at an exact place, from a save's lines | modes_surface.cpp |
| `consistency` |  | N0-02: for every landable type the landing map, the globe and the ground at one point | modes_surface.cpp |
| `drive` |  | N4-02: the buggy driven on open ground, frames from the seat | modes_surface.cpp |
| `bestiary` |  | N3-01: the bestiary of the first living worlds near the start, made twice | modes_surface.cpp |
| `encounters` | `[n]` | N3-04: the share of n random felisian sites with a herd within 200 m, by biome | modes_surface.cpp |
| `landmaps` |  | the landing map of a body of each landable type, its darkest shadow | modes_surface.cpp |
| `stability` | `[scene] [metres per frame] [scale] \| flight [scale] \| skyline [scale]` | B-309: what moves in a scene's frames; G-07: the band in the flights; A-02: the city's skyline | modes_surface.cpp |
| `compare` |  | B-310: eight scenes in six configurations of the render scale and the mush | modes_surface.cpp |
| `sheet` |  | B-310: compare's frames as the window shows them, a sheet a scene | modes_surface.cpp |
| `gallery` | `[type]` | M9-01: sixteen random landing sites of a type as a contact sheet | modes_surface.cpp |
| `reanchor` | `[comet [scale]]` | KI-007: a 51 km walk across the site's re-anchor; F-01: the comet's re-anchor timed | modes_surface.cpp |

## terrain

| Mode | Arguments | What it does | File |
| --- | --- | --- | --- |
| `terrain` | `[type]` | O6-01: 48 land sites a landable type on four grids, judged by relief class | modes_terrain.cpp |
| `landforms` | `<type> [flat\|hills\|mountains] \| sites` | O6-01: the landform gallery, or the fixed review sites | modes_terrain.cpp |
| `descent` | `[latDeg lonDeg]` | B-307: the buggy down the steepest descent at the pinned mountain site | modes_terrain.cpp |
| `matlod` | `[all\|<type>] [bodies] [sites] [halfKm] [stepM] \| spot <sx> <sy> <sz> <body> <latDeg> <lonDeg> [halfKm] [stepM]` | B-404: the materials and heights at the five ring scales, point by point | modes_terrain.cpp |
| `landmarks` | `[type]` | O6-06, R-408: the sights round 24 random land sites of a type | modes_terrain.cpp |
| `drain` | `[type]` | O6-03: the drainage round the first land sites of a type: its cost, channels and lakes | modes_terrain.cpp |
| `water` | `[n]` | B-320: the inland water of the first n felisian worlds at 16 m | modes_terrain.cpp |
| `climate` | `[n]` | B-321: the first n felisian worlds' climates, biome shares and land ice | modes_terrain.cpp |

## kinds

| Mode | Arguments | What it does | File |
| --- | --- | --- | --- |
| `kinds` | `[name\|ground\|family]` | P-00: a sheet a kind of world (or a ground's kinds, a family's) | modes_kinds.cpp |
| `traits` | `[name\|new\|karst]` | R-304, Q-01: the first body with each landform trait, its sheet | modes_kinds.cpp |
| `tides` | `[scene]` | Q-02: the great tides of the scan region's first worlds that have them | modes_kinds.cpp |
| `datasheet` | `[n]` | Q-01: the n data sheets that run longest, in space and on the ground, a page a frame | modes_kinds.cpp |
| `salt` | `scan [name\|ground\|family] [tries]` | E-07: the made kinds against the protected worlds as kindOf walks them: what each reaches and takes, the first salt taking none | modes_kinds.cpp |

## life

| Mode | Arguments | What it does | File |
| --- | --- | --- | --- |
| `life` | `list [n] \| survey [systems] \| kin [ly] \| plates [n] \| strips [plan] \| distinct [species] \| forms [plan] \| search [scene\|all] \| kit \| made [n] \| auto \| sight` | Z-01: the biota of the worlds nearest home; by kind over the scan region; the geneses and the kin cells. Z-02 (modes_life_look.cpp): the forms' plates, the acts' strips, no two alike, every plan's variants in the six ways. Z-03: each microhabitat round the search's scenes and what a sample there holds, read again from another landing; the kit played (its frames). Z-06 (modes_tree.cpp): `made`, the made species of the late peoples' worlds nearest home and fifty of their watermarks as the decoder reads them, the checks, the repeats at a hundred. Z-07: `auto`, auto-explore's water outing on a felisian world and at a terraced spring, run till it takes a sample (its steps, the vial's line). R-434: `sight`, the activity vision played on a superhabitable world's shelf (on, from the buggy, a vial where it shows strong, off) | modes_life.cpp |
| `lab` | `microscope \| time \| cultures \| jars \| tree \| kin [r] \| catalogue \| genes` | Z-04: the microscope opened on samples of each stage and chemistry: every way of seeing at every zoom, a cell tracked, the light's crowd, the magnet's turn, the warm stage, the food, a fossil's section; the mounts and the sheets. `time`: a dense slide's frame timed by its parts. Z-05 (modes_cultures.cpp): `cultures`, twenty species an hour in four media (their curves, their stages' times), the pairs (the cycles' periods, the winners, the mirrored), the incubator's screen; `jars`, worlds' jars at 0, 5, 10 and 20 minutes and their shelf. Z-06 (modes_tree.cpp): `tree`, a sequenced set (the flow's world's, home's, a kin star's, its own genesis's, a mirrored one, a drifter's and the star it left, a made one on the sequencer), the tree printed and drawn, the decoder's watermark, the kin near home; `kin`, those alone. Z-07: `catalogue`, a guide as the lab fills it and a friend's file lent: the catalogue by world (the lent in cyan) and by kin, a sheet's form live, a name given from it; the statistics' LIFE page, the data sheet's microbes, a micrograph in the gallery. Y-02 (modes_genes.cpp): `genes`, a world's species' genes for each alphabet and hand (each genesis's map and code), every gene's edit and cut on a species (its values before and after), the gene editor's screen (`lab_genes.png`: its chooser, its map, a known gene's letters changed, its line writing and in its flask), the genes' laws | modes_lab.cpp |

## radio

| Mode | Arguments | What it does | File |
| --- | --- | --- | --- |
| `signals` | `[<sx> <sy> <sz>] [reach] [xG] \| wav` | C-07: the far signals heard from a sector; their WAVs | modes_signals.cpp |
| `voice` | `<sx> <sz> <body> [index] [wav]` | C-05: a world's voice, a record spoken | modes_signals.cpp |
| `sounds` |  | Q-03: the ground's sounds through the synth: the dunes, the ice, a quake | modes_signals.cpp |
| `rogues` | `[ly] \| heard \| voices \| dark` | D-01: the drifters near home; D-02 to D-04: heard, their voices, their dark | modes_rogues.cpp |

## peoples

| Mode | Arguments | What it does | File |
| --- | --- | --- | --- |
| `peoples` | `ages \| fresh \| map \| scorched \| rogues \| <sx> <sz> <body> [second]` | C-13, A-01: the peoples near home by age, a world's peoples, the scorched worlds' and the drifters' | civ_peoples.cpp |
| `history` | `<sx> <sz> <body> [second] \| ages \| check` | H-02: a people's history, printed as legends | civ_peoples.cpp |
| `genome` | `sheet \| street \| dress \| ages \| reach \| distinct \| pair \| print \| ...` | A-04: the peoples' genome, its architecture sheets | civ_genome.cpp |
| `forms` | `sheet \| city \| skyline \| stability \| check [scale]` | A-02: the forms' sheet, the test city, its skyline | civ_forms.cpp |
| `plan` | `<sx> <sz> <body> [second] \| ages \| view` | T-01: a world's plan for each age | civ_plan.cpp |
| `towns` | `<sx> <sz> <body> [second] \| ages \| view \| check` | T-02: a people's towns | civ_plan.cpp |
| `roads` | `<sx> <sz> <body> [latDeg lonDeg]` | C-09: a world's roads | civ_plan.cpp |
| `works` | `<sx> <sz> <body> [second] \| ages \| map \| gallery \| view \| rogue <k> [ly] \| check` | T-03: a people's works | civ_works.cpp |
| `ruins` | `<sx> <sz> <body> [latDeg lonDeg] \| ages \| kinds \| check` | C-01: a world's culture and the ruins round a point | civ_lore.cpp |
| `shards` | `<sx> <sz> <body> [second] \| drifters [n] [held] \| rogue` | C-02: a people's fifty records as the decoder reads them | civ_lore.cpp |
| `charts` | `<sx> <sz> <body> \| ages \| rogues \| check` | C-10: the star charts, in each age's way | civ_lore.cpp |
| `graves` | `<sx> <sz> <body> [latDeg lonDeg] [cells]` | C-12: a world's calendar, its last recording and its graves | civ_lore.cpp |
| `music` | `<sx> <sz> <body> [index] [wav] [age=N] [tone=N] \| ages [wav]` | C-04: a people's music | civ_music.cpp |
| `records` | `<sx> <sz> <body> [second] \| ages \| scenes \| check` | T-04: a people's records and where they lie | civ_records.cpp |
| `decoder` | `[ages] \| counts \| pages \| wav \| check` | H-05, H-06: the decoder's counts, pages and media | civ_records.cpp |
| `instruments` | `[ages] \| <sx> <sz> <body> [second] \| scenes \| sheet \| plans \| distinct \| wav \| check` | C-11, C-17: the peoples' instruments, their makers and the player | civ_instruments.cpp |
| `grammar` | `[dir D] [embedded] [near N] [print N\|all] [rogues[=ly]] \| check \| where [<rule>\|<prefix>.*\|check\|md] \| schema` | H-03: the records' grammar read from the tree; E-11: where a rule is, what it may name | civ_grammar.cpp |
| `tongues` | `[n]` | H-04: the tongues of the n peoples nearest home | civ_grammar.cpp |
| `walls` | `forms [peoples] [years ...] \| one \| at \| place \| reach \| view \| walk \| check` | B-420, B-421: every piece against its colliders | civ_walls.cpp |

## game

| Mode | Arguments | What it does | File |
| --- | --- | --- | --- |
| `flow` |  | the scripted expedition through every screen and device; read its status lines | modes_game.cpp |
| `settings` |  | the settings file round trip and the settings screen | modes_game.cpp |
| `audio` |  | M4-09: every family of sound through the synth into a WAV | modes_game.cpp |
| `input` |  | M6-02, R-418: the key map and the controller | modes_input.cpp |
| `tour` | `[state ...]` | U-01: every state opened as the game opens it, its controller menu, keys and rows; U-06: then every row, its commands | modes_tour.cpp |
| `fuzz` | `[n]` | M0-08: random keys and mouse for n frames (10,000) across every state | modes_input.cpp |
| `zoom` | `<sx> <sz> <body> <latDeg> <lonDeg> [scale]` | C-09: the landing map's zoom window over a point | modes_game.cpp |
| `drones` | `[frames] \| check \| world <sx> <sz> <body>` | C-18: the drone bay in a game | modes_drones.cpp |
| `ship` | `rooms [scale ...] \| walk \| light [scale] \| life [scale]` | I-02: each space from its arch and its middle; a walk to every device and through every arch; I-03: the light (lit and dark, the sun's patches, the hull outside); I-04: life aboard (the photographs, the garden, the shelves, the core, the hatch's drop) | modes_ship.cpp |
| `parkings` |  | B-428: twelve parkings of the scenes that were crowded | modes_game.cpp |
| `tape` | `[dir]` | R-420: auto-explore's tape, its tracks and the deck's sequences | modes_game.cpp |
| `tail` | `[<sx> <sy> <sz> <body>]` | B-427: a Wolf-Rayet star's giant and the tail its wind strips | modes_game.cpp |
| `auto` | `[profile] [minutes] [seed] [cockpit]` | R-419: an expedition played by auto-explore | modes_game.cpp |

## proof

| Mode | Arguments | What it does | File |
| --- | --- | --- | --- |
| `unit` | `[list] [-q] [group ...]` | the unit's checks, all or by group (`unit list`); -q: only the failures | unit.cpp |
| `regress` | `[bless]` | M0-03: the determinism hashes against tests/regress_baseline.txt (bless: with GEN_VERSION only) | modes_proof.cpp |
| `fingerprint` | `bodies \| stream \| peoples \| modules \| life [n] [out] [full]` | E-02: what the generators make, as keyed lines (tools/prove) | fingerprint.cpp |
| `screens` | `[scale ...]` | F-05: the game's frames the regress never hashes | modes_proof.cpp |
| `pngdiff` | `<a.png> <b.png> [diff.png]` | F-01: two PNGs pixel for pixel | modes_proof.cpp |
| `bothways` | `<scene> [scale]` | F-01: a scene by the workers and by one thread | modes_proof.cpp |
| `suncone` |  | F-05: the sun's cone test swept to the frame's corner | modes_proof.cpp |

## bench

| Mode | Arguments | What it does | File |
| --- | --- | --- | --- |
| `bench` | `[check [slack]] \| forest \| noise` | the frame's costs; `check` against the 46 budgets | modes_bench.cpp |
| `pool` |  | F-06: the pool's dispatch timed and its dealing checked | modes_bench.cpp |
| `load` | `landing \| map \| arrival \| drive \| capital \| probe \| roads [seconds] [scale] [age]` | B-422: the cores kept busy and the pace | modes_bench.cpp |
| `soak` | `[minutes] [scale]` | F-01: a long expedition at 4x | modes_bench.cpp |

## harness

| Mode | Arguments | What it does | File |
| --- | --- | --- | --- |
| `help` | `[md]` | this list; `md` as the docs' index | main.cpp |
