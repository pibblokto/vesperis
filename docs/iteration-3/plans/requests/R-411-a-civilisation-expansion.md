# R-411: a civilisation expansion: ages, architecture, places, histories and records

status: planned 2026-10-08 (blocks A, T and H of `docs/archive/ideas/ITERATION-1.3.0.md`); H-01 and A-01 done 2026-10-08, A-02, A-03, A-04, T-01, H-02, T-02, T-03, T-04 and H-03 done 2026-10-09, H-04, H-05 and H-06 done 2026-10-10 (the three blocks done; C-11, the instruments, closes the release)
milestone: 1.3.0

## Request

"Another thing I would like you to plan for release - huge civilization expansion, in three blocks/sets of milestones. First
one - there can be several stages of civilization at which you find them at their collapse - prehistoric, medieval,
victorian-ages like, moderm days, futuristic, far-futuristic advanced. Next - each stage should be represented in what
structures are present, how are they built, density, etc. Next - even for each stage there should be complex and interesting
architecture radnomizer so that civilizations are TRULY uniqie in terms of buildings (not just placement but different forms,
accents on what to look at, colors, everything and more, you also add dozens of variables that randomize it and make unique per
civilization). Next - huge improvements on how setlements placed, how cities/towns/villages look like, in some cases it may be
something else completely, like people don't just build monuments and villages, it can be virtually anything (don't forget
shards placement). Next - there should be huge overhaul of text generation using compltex free-context grammar with dozens of
inputs so that it's like complex dwarf-fortress level texts. And therye should be far greater varation but again -
investigation on free-context grammar is required, dozens of inputs, different reasons for collapse, different records,
something really dwarf-fortress like in complexity. Also there should be more shards to find but they should have different
categories - music/text/start charts (also stylize them per civilization stage). This context-free grammar stuff would also
need investigation and research on your side, include it in milestones"

What exists is the C series of 1.1.0: one felisian or desert world in eight had a people; its settlements are hashed cell by
cell on a 2 km grid, ten building kinds of boxes and domes in a `Culture` of about a dozen numbers; fifty shards a people, texts
of about 31 words from a hundred templates (`docs/reference/civilisations/`).

## Plan

`docs/archive/ideas/IDEAS-civilisation-expansion.md` holds the design and its lists; the milestones are in the iteration file:

* **A, the ages and what they built:** A-01 the ages (a people's age at its collapse, its profile, who transmits; generation
  13), A-02 the forms the renderer needs, A-03 what each age builds (decay by material, older layers), A-04 the architecture
  genome (thirty-five deep variables, the sheet, distinctness measured).
* **T, the towns and the works:** T-01 a people's map of its world, T-02 towns by age, T-03 the works ("virtually
  anything"), T-04 where the records lie.
* **H, the histories and the records:** H-01 the research into grammars and histories (first of the three blocks), H-02 a
  simulated history per people, H-03 the grammar engine, H-04 the records (genres by age, 30-400 words, dozens of inputs, the
  collapse from many sides), H-05 more shards in three categories styled by the age, H-06 the decoder reads a history.

## Done

* **H-01, the research (2026-10-08).** `docs/archive/ideas/RESEARCH-records.md`: the survey of the sources the ideas note named, the
  seven questions answered, the formalism (a history told by a small attribute grammar over a realiser) and the authoring
  format (`.vg` files embedded as raw strings, reloaded by the harness) with the reasons, the evaluation plan with targets,
  the inputs list and the data model the peoples' milestones build to. `docs/archive/ideas/RESEARCH-records-samples.md`: the scratch
  prototype's fifty records (letters, chronicles, epitaphs) on the two test worlds beside C-02's texts, one plague from
  three sides, the measures. The prototype is out of the tree; nothing in the game changed.
* **A-01, the ages (2026-10-08).** Every people has the age it had reached when it collapsed and a profile built to H-01's
  data model (`galaxy/peoples.*`, `docs/reference/civilisations/peoples-and-ages.md` "The ages"; generation 13):
  * the six ages by the decided shares (28/32/18/13/7/2%);
  * the eras with their years in the people's own count and the government each closed with;
  * radio (29% of all peoples);
  * the end's years by the age, longer for the late ones, with the ruins weathered by the age's durability;
  * one of 37 causes by the age and the world, shared by two peoples that ended together, with the collapse's length and
    four stages;
  * the land of their time, the share held, how many they were;
  * government, economy, faith, rite, holy place and five values, all named for the grammar.

  Only a people that had radio transmits, so C-07's candidates went up 3.4 times and the salt was pinned again: home hears
  three peoples of three ages. `vesperis_test peoples ages` lists the hundred nearest peoples, the shares and a pinned test
  world of each age. No screen names an age (the plan's decision); the ruins, the records and the broadcasts follow it in
  later milestones (KI-369, KI-370).
* **A-02, the forms the renderer needs (2026-10-09).** The shapes the ages will build with, in the renderer and the harness
  (`galaxy/forms.*`, `surface/forms_draw.cpp`, `docs/reference/civilisations/built-world.md` "The forms the renderer needs"): twelve
  kinds (C-01's box and dome, domes of four profiles, cylinders, cones, spires, a cooling tower's waist, tanks, chimneys,
  prisms, pyramids and stepped masses, vaults, arches and arcades, saddles and dishes, trusses, cables, rings, curved walls,
  amphitheatres, dams, grown forms), each with its broken state; a level of detail a building down to a silhouette on the
  horizon; colliders; a people's colours in four ramps and its windows as a pattern on a face. The game's settlements stay
  C-01's until A-03 builds the ages with these. KI-373.
* **A-03, what each age builds (2026-10-09).** Every people's settlements are built as its age built, from A-02's forms
  (`galaxy/buildings.*`, `docs/reference/civilisations/built-world.md` "What each age builds"): huts, longhouses, tent rings, stone
  circles, standing stones, dolmens and palisades; C-01's stone houses roofed, temples with towers and spires or domes and
  minarets, keeps, walls with round towers and gatehouses; brick terraces, tenements, villas, churches, town halls, stations,
  mills with their chimneys, gasometers, headframes; blocks, suburbs, towers of 100-500 m, stadiums, masts, cooling towers;
  arcologies, habitat domes, spires over a kilometre, rings, maglev lines, pods, wrecks; the far future's one structure. They
  stand in C-01's plans until T-02 (their spacing and how many cells hold one by the age) and weather by their materials: a
  wooden village after a thousand years is rings of post holes, a concrete tower after three thousand a stump, a far-future
  spike after a hundred thousand as sharp as in its first year. Every building with a room says where a record can lie; the
  shards lie there. A scene of each age, `ruins ages` and `ruins kinds` to see them. KI-374.
* **A-04, the architecture genome (2026-10-09).** Every people draws thirty-five deep variables from its seed and keeps them through
  its ages (`galaxy/genome.*`, `docs/reference/civilisations/built-world.md` "The architecture genome"): how it massed and proportioned its
  buildings, their plans, roofs, walls' profile, symmetry and sacred number, its windows' shape, rhythm and share, its ornament,
  motif, structure and tops, its colours (a base, an accent and where it goes, its finish, its glass, its light), its relation to the
  land, its facing, streets, public spaces, grouping, edge and links, its focal accent, its centre, its monuments, its dead, how alike
  its houses are and whether it built to last; its age's technique by their leanings. A people of blue spires raises one over its
  huts, its church and its tower block; a people of red domes builds them in stone, brick, concrete and composite. The architecture
  sheet sets the twelve peoples nearest home on one ground under one light, a page an age, their genomes printed; a people's towns in
  every age side by side; the hundred nearest are measured distinct (the nearest pair 4.93 apart on a scale of 13.4, alike under 3.4);
  the unit checks that the genome is deterministic and every variable reaches the drawing. KI-375.
* **T-01, a people's map of its world (2026-10-09).** "Huge improvements on how settlements placed": a people's world has a plan
  (`galaxy/plan.*`, `docs/reference/civilisations/built-world.md` "A people's map of its world"). The world read coarsely (land, height,
  roughness, climate, fertility, coasts, rivers, coal and ore) and scored as the age would settle it, by the people's relation to the
  land too; its territory grown from its capital to the share it held; its capital, cities and towns by rank (a prehistoric people's
  gathering place and seats, a futuristic people's few great places, the far future's one to three works), spaced as the age
  travelled, each with its founding rank, its population and the core the ground draws; its networks (paths, roads, rail and canals,
  highways, maglev on pylons) joining every place, laid on the ground by their kind. The cells read it: the countryside only in the
  land the people held, a city's core drawn whole across cells and far cities on the skyline; the landing zoom and the telescope show
  the cores and the ways, the scanner hears a capital from 400 km. Built once a world on a thread at the approach. KI-376.
* **H-02, the history (2026-10-09).** "Dwarf-fortress level", which H-01 found is mostly in what a text tells: each people has a
  history now (`galaxy/history.*`, `docs/reference/civilisations/peoples-and-ages.md` "The history"), simulated a year at a time over the plan's
  places before the towns, the works and the records that will read it. Its eras with their names, governments and faiths; its places
  founded in the plan's order in the era whose ground suited them and left by the end in its cause's order; its rulers, houses, heirs
  and regents, its generals, prophets, makers, inventors and explorers, ten families with their births, marriages, moves and deaths;
  its wars and battles, sicknesses, famines, the world's own disasters, works, inventions, discoveries, institutions and omens; two
  peoples' meetings, treaties and wars, and a conquest; the collapse in its four stages to its last person at its last place, every
  event's cause kept. `vesperis_test history` prints it as Dwarf Fortress's legends would. KI-377.
* **T-02, towns by age (2026-10-09).** "Huge improvements on how settlements placed, how cities/towns/villages look like": a village, a
  town and a city are laid out as their ages laid them (`galaxy/towns.*`, `docs/reference/civilisations/built-world.md` "Towns by age"): streets
  first in the people's pattern and the age's way (crooked medieval lanes inside a wall with its gates, an industrial quarter's straight
  streets and back lanes round its mills and station, a modern city's blocks, ring road and suburbs' cul-de-sacs), the buildings fronting
  them by district (the centre, the old core, the houses, the works, the harbour, the towers, the suburbs), the height and the density
  falling from the centre; a city in rings of the ages the history says it lived through (a modern capital's medieval core and its old
  wall's line), a place left early in its older way; the harbour, the water and the fields by age (a quay, a dock and its cranes, a
  container port; cisterns and reservoirs; strips, enclosures, irrigation circles); the streets, squares and fields drawn into the ground.
  `vesperis_test towns` draws the plans from above, `towns view` through the renderer. KI-378.
* **T-03, the works (2026-10-09).** "It can be virtually anything (don't forget shards placement)": twenty-one kinds of works outside the
  towns (`galaxy/works.*`, `docs/reference/civilisations/built-world.md` "The works"), each where its rule of the terrain or its history puts it and
  in its age's way: quarries in the towns' rock, mines, dams at a valley's narrows holding their reservoirs, aqueducts on their arches,
  observatories on the summits, a shrine at the holy place and its pilgrims' way, necropolises facing their cities, lines and figures on a
  dry plain, henges and stone rows, a fortified line on the frontier, a battlefield of each war's greatest battle between its two sides,
  launch sites near the equator, antenna fields, wrecks, a colossus, terraformed basins, the last shelter by the last place, wind farms,
  lighthouses, and the far future's inexplicable; their ground drawn into the terrain; the scanner's WORKS mode hears them. The records'
  places in them are T-04's. `vesperis_test works gallery` shows each kind of each age. KI-379.
* **T-04, where the records lie (2026-10-09).** "Don't forget shards placement": a people left 80 to 200 records by its age and its numbers
  (`galaxy/records.*`, `docs/reference/civilisations/built-world.md` "Where the records lie"), C-02's fifty the first, each dated in its history and
  made in its year's way (carved bone and painted walls to codices, wax cylinders, discs, drives, crystals and something that hums), drawn
  as that object and taken with `E`; each kept at a home among its people's places, copies elsewhere, archives in the capital and the
  cities, records in the works that kept them, the last recording at the history's last place and in its last shelter. The shards found
  before are kept by their index. `vesperis_test records scenes` shows each age's objects drawn and taken. KI-380.
* **H-03, the grammar engine (2026-10-09).** "This context-free grammar stuff would also need investigation and research": the formalism
  H-01 chose, built (`galaxy/grammar.*`, the format in `docs/reference/civilisations/records-and-grammar.md` "The grammar engine"): plain `.vg` files in
  `src/galaxy/grammar/`, compiled in and read from disk by the harness, rules with weights and guards, persons the realiser refers to (a
  pronoun only where it is clear), verbs, numbers, plurals and articles written by the realiser, the checks (a doubled word, an article
  before a plural, a pronoun against its verb, a length, an anachronism by an age's lexicon...) and the measures. C-02's kinds were
  rebuilt on it first: `vesperis_test grammar` prints 3,479 texts and 444 captions of 48 peoples with every check at zero, 3.9 us a
  text. KI-381.
