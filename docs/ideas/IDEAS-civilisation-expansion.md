# Ideas: the civilisation expansion (the ages, the places, the records)

Planned 2026-10-08 on the user's request (`docs/iteration-3/plans/requests/R-411`), for 1.3.0 as three
blocks of milestones (`ITERATION-1.3.0.md`): **A, the ages and what they built**; **T, the towns and the
works**; **H, the histories and the records**. Nothing here is built. This note holds the intent and
the lists; the iteration file holds the milestones, their checks, the order they are built in (the
blocks interleaved with the galaxy chart and the radar) and the decisions taken with it.

The request, in the user's words: several stages of civilisation at which a people is found at its
collapse ("prehistoric, medieval, victorian-ages like, modern days, futuristic, far-futuristic
advanced"); each stage shown in what structures are present, how they are built, their density;
"complex and interesting architecture randomizer so that civilizations are TRULY unique in terms of
buildings (not just placement but different forms, accents on what to look at, colors, everything and
more ... dozens of variables)"; "huge improvements on how settlements placed, how cities/towns/villages
look like ... it can be virtually anything (don't forget shards placement)"; "huge overhaul of text
generation using complex context-free grammar with dozens of inputs ... dwarf-fortress level texts",
bigger texts, far greater variation, different reasons for collapse, different records, the grammar
researched first; more shards, in categories (music, text, star charts) styled per stage.

## 0. What exists (the C series, 1.1.0)

One felisian or desert world in eight had a people (`TR_CIVILISATION`), one such world in five two
peoples (C-13). Every 2 km cell of the world's grid is hashed on its own: 35% hold a settlement, a
hamlet, village or town of ten building kinds (house, hall, tower, rotunda, gate, platform, colonnade,
stela, rubble, wall) in five plans, built of boxes and domes (`RuinElem`), with C-08's seven features
read from the terrain (quays, moles, bollards, terraces, cisterns, wind walls, cairns), decayed by the
people's age since the end (200-40,000 years) and the rain. A people's `Culture` is about a dozen numbers
(smooth or striated walls, rock or adobe, tall or squat, stone or timber roofs, the plan, towers or none,
how deep the sand buried it, the age, the roads). Settlements are drawn within two cells (about 5 km),
in the ground's own rock or sand bank. Roads (C-09) join the settlements within two cells. A people has
fifty shards: 12-18 pieces of music, 3-5 star charts, the rest texts of about 31 words from thirty-one
kinds and a hundred templates (`{slot}` and `[a|b]` marks over some twenty slots read from the world,
then `tidy`). Graves (C-12), a calendar, the last recording, the decoder, the voice and the radar
(C-07: a people's world transmits when its sector is a transmitter) all read the same people.

What this gives: a dead people's world looks like any other's but for its proportions and its
material, and its texts, after a few dozen, show their templates. The expansion is about difference:
between ages, between peoples of one age, between the places of one people, between one record and the
next.

## 1. The ages (A-01)

A people is found at the age it had reached when it collapsed. Six ages, the user's:

| Age | What it was | Built with | Its records on | On the air |
| --- | --- | --- | --- | --- |
| Prehistoric | bands, herders, the first fields and villages | wood, hide, thatch, earth, dry stone, megaliths | carved stone and bone, painted rock | nothing |
| Medieval | kingdoms and temples, walled towns, mills, ships of wood (it stands for every hand-made age) | stone, brick, timber, tile | tablets, scrolls, codices | nothing |
| Industrial (the user's "Victorian") | coal, iron, rail, the telegraph, the first century of machines | brick, iron, glass, cast stone | print, photographic plates, wax cylinders | late in it: spark gaps, the first voices |
| Modern | electricity, oil, concrete, towers, aircraft, broadcasting | concrete, steel, glass, asphalt | tape, film, discs, the first data | voices, music, numbers |
| Futuristic | fusion, orbit, arcologies, machines that think | composites, ceramics, grown stone, diamond glass | data crystals | data, synthetic voices, beacons |
| Far future | matter shaped to purpose; what we cannot name | one material over kilometres; nothing that weathers | things that keep themselves | a beacon that is mathematics |

* **A people passed through the ages before its own.** (B-414, 2026-10-10, the user's call: superseded. A world shows only the age its
  people reached; its earlier ages live in its history and in the histories its records keep: `docs/reference/10-decisions.md`, B-414.)
  A modern people's world keeps its medieval towns
  as the old cores of its cities and its megaliths in the fields, each in the decay of its own time; a
  far-future people's world may show almost nothing of its earlier ages (they cleared it) or keep it as
  a museum would.
* **The profile.** Drawn from the people's seed and its world: the age reached, the eras it passed, the
  cause of its collapse (section 8), how long the collapse took, how long ago it ended (C-08's 200-40,000
  years, and longer for the late ages, whose works outlast it), how many they were and how much of the
  world they held. One function every block reads.
* **Who transmits.** Only peoples that had radio can be on the air: the late industrial age and after.
  C-07's transmitter rule gains the age; the salt is re-pinned so home still hears signals.
* **Shown, not told** (decided 2026-10-08 with the order): no screen names an age; the ruins show it
  and the records tell it, and the decoder's history page (H-06) names it once read.
* Shares: the early ages commoner, the far future rare (most peoples never get far; a far-future world
  should be an event).

**Done (2026-10-08, A-01):** `galaxy/peoples.*`, generation 13 (`docs/reference/12-civilisations.md`, "The ages").
* The shares are 28, 32, 18, 13, 7 and 2 per cent, drawn from the people's own generator whatever the world.
* The eras carry their years in the people's count (year 1 the first towns, earlier ages negative) and each its government.
* Radio comes late in the industrial age (four industrial peoples in ten had it) and in every later age: 29% of all.
* The end keeps C-08's draw over the age's span, longer for the late ages, and the ruins weather by the years over the age's
  durability until A-03.
* There are 37 causes by age, weighed by the world (the drying on a desert, flares round a red dwarf, impacts by the
  comets and belts...); two peoples that ended together share one. The collapse's length and stages follow the cause.
* The extent (the land of their time, the share held, how many), the government, economy, faith, rite, holy place and five
  values complete the profile, all with names for the grammar.
* Only a people that had radio transmits: C-07's candidates went up 3.4 times and the salt was pinned again. Home hears a
  modern people at 90 ly, a futuristic one at 184 and an industrial one at 197.
* A test world of each age is pinned (`vesperis_test peoples ages`).
* KI-369 and KI-370 hold what waits for the later milestones.

## 2. What each age builds (A-03)

| Age | Dwellings | The civic and the sacred | Works | Density and height |
| --- | --- | --- | --- | --- |
| Prehistoric | rings of huts, longhouses, pit houses, cliff dwellings, tent rings | stone circles, menhir rows, dolmens, barrows, painted caves | hunting walls, fish weirs, flint pits, cairns on the passes, middens | camps of a handful, days apart; 2-10 m, megaliths to 20 |
| Medieval | rows of houses, closes, farmsteads | temples, castles and keeps, palaces, monasteries, market halls, gates and walls | mills on rivers and hills, arched bridges, harbours, terraces, aqueducts, field walls | villages a walk apart, towns of thousands in walls; houses 5-10 m, the temple 60-100 |
| Industrial | terraced rows, tenements, villas | town halls, stations, exchanges, museums, observatory domes | mills with chimneys, rail on viaducts, canals with locks, iron bridges, gasometers, headframes and spoil heaps, docks and cranes, lighthouses, telegraph lines | cities of hundreds of thousands; 10-30 m, chimneys and spires to 100 |
| Modern | blocks, towers, suburbs | towers, stadiums, hospitals, airports, broadcast masts | highways and interchanges, suspension bridges, dams, power stations, container ports, open pits, irrigation circles, wind farms, radar domes, silos, launch pads | coastal megacities of millions, suburbs; towers 100-500 m |
| Futuristic | arcologies, domes, vertical towns | spires, the ring | maglev on pylons, launch loops and tethers, seawalls, terraformed basins, fusion plants, vertical farms, wreckage fallen from orbit | a few great places and the land between left wild; to kilometres |
| Far future | none as such | single structures | a ring round a mountain, a spike, a plain made flat for a hundred kilometres, pillars to the horizon, a shaft into the crust, a lattice over a valley, the land itself shaped | one to a few on a world; they do not decay |

**Decay by material** (replacing C-08's one decay with the material's, still by the age and the rain):
wood, thatch and hide gone in centuries (post holes, rings of stones); dry stone and megaliths standing
tens of millennia (leaning, fallen); mortared stone and brick C-08's model (walls for centuries, falling
over millennia); iron and steel rusting to skeletons, then stains; concrete spalling within a century,
its bars showing, towers falling over centuries to a few millennia while dams and bunkers last; glass
gone at once (empty frames); composites lasting millennia, then crumbling; the far future's materials
not weathering at all, a sharp edge after a hundred thousand years. Wet worlds overgrow (KI-350's
rounded walls), deserts bury (C-01's drifts).

**Done (2026-10-09, A-03):** `galaxy/buildings.*` and the plans in `galaxy/ruins.*` (`docs/reference/12-civilisations.md`, "What each
age builds").
* The table's dwellings, the civic and the sacred buildings, and the works that stand in a town, about forty-five kinds, each a
  generator of A-02's forms: so a people's older works can be built in their own age's way (T-02 places them).
* The density and the heights: C-01's plans (until T-02) spaced by the age, the share of cells holding a settlement by the age
  (the far future's one structure in a cell of about seven hundred: one to a few on a world waits for T-01); houses of 5-10 m and
  temples to 60-100, chimneys and spires to 100, towers of 100-500 m, futuristic spires over a kilometre, a far-future spike of
  two.
* The decay by material, by the age and the rain: ten materials, each with its years to half gone, its spread and its rain;
  masonry C-08's to the digit; nothing rests on what fell; tops lie where they fell; the glass is gone at once.
* Rooms where the records will lie (T-04): every building's floor, doorway and spots; the shards lie there now.
* Not done here: overgrowth on wet worlds (a young ruin is a clearing the trees take back), rust stains, a concrete stump's bars
  (KI-374).

## 3. The architecture genome (A-04)

Every people draws a genome: the deep variables are its own and kept through its ages (a people that
loved domes builds them in stone, in concrete and in glass; its castle and its towers are one people's),
the age's variables are what its age could do. The randomiser is the genome; the uniqueness is measured,
not hoped for.

**The deep variables** (thirty-five):

* *Mass and proportion:* massing (compact, sprawling, vertical, buried, stepped); proportion (squat to
  slender); scale ambition (the greatest building against a house); appetite for height across the ages.
* *Form:* the plan's form (rectangle, circle, polygon of the sacred number, ellipse, free); the roof
  (flat, pitched, hipped, domed, conical, vaulted, saddle, stepped, open); the wall's profile (upright,
  battered, bulging, leaning out, stepped back); symmetry (bilateral, radial, rotational, none); the
  sacred number (columns in a row, a polygon's sides, tiers); modularity (cells repeated, or one-offs).
* *Surface:* the openings' shape (square, arched, round, slit, triangular, keyhole); their size and
  rhythm (regular, paired, grouped, scattered) and their share of the wall; ornament's density (bare,
  banded, carved, encrusted) and motif (lines, circles, steps, spirals, glyph bands, figures, stars);
  the structure's expression (heavy masonry, an expressed frame, smooth monolith); the tops (finials,
  crenels, masts, statues, nothing).
* *Colour and light:* the base colour, the accent and where it goes (bands, roofs, doors, tops); painted
  or the material's own; glazing tint in the later ages; light (glowing lines, lamps, none: a dead city
  is dark, a far-future one may still glow).
* *Place:* the relation to the land (ridges, valleys, terraced into slopes, cut into cliffs, on stilts
  over water, on mounds, sunk into pits); orientation (to the sun's rise, a star, the moon, the river,
  the wind, none); the street pattern (grid, radial, concentric, organic, along a line, terraced on the
  contour, branching); public space (plazas, inward courts, gardens, none); grouping (compounds round a
  court, rows, detached); the edge (walls, ditches, nothing); connections (arcades, bridges between
  towers, tunnels, colonnades).
* *Meaning:* the focal accent, what the eye goes to (a spire, a dome, a gate, a tower, a colossus, a
  mast, a ring, an obelisk, a pool, a great tree); the centre (temple, palace, assembly, market, tomb,
  machine); monuments (stelae, obelisks, colossi, arches, pillars); the dead (barrows, tombs,
  cemeteries, towers of silence, nothing); sameness or variety from house to house; built to last or
  built light (which ages' works survive).

**The age's variables:** the materials, the techniques (post and lintel, arch and vault, frame,
cantilever, tension, shell, printed or grown), the heights and spans it allows, the building kinds of
section 2, the density, the networks.

**Two rules.** At 320 x 200 through the mush a people is told by its silhouettes, proportions, rhythm and
colours, not by fine ornament: every variable must act at a scale the picture shows (judged on frames
at 1x-4x, not on numbers). And colour lives in banks: a people gets a palette of three or four ramps
(the structure, the accent, metal and glass, light) in the banks the surface leaves free (18-20 and
24-31 by the bank table; the ruins draw in the ground's rock or sand bank today).

**The proof** is a sheet: a dozen peoples per age at the same light, with their genomes printed, and a
measure of distinctness (no two of a hundred near home alike on the strongest variables).

**Done (2026-10-09, A-04):** `galaxy/genome.*`, the genome's ways in `galaxy/buildings.*` and its plans in `galaxy/ruins.*`
(`docs/reference/12-civilisations.md`, "The architecture genome").
* The thirty-five deep variables as listed (the accent and where it goes one variable; the size and rhythm of the openings two: the
  rhythm and the glazing's share), a hash each from the people's seed, kept through its ages; the age's technique drawn among the
  age's by their leanings (post and lintel; arch and vault; frame; cantilever; tension; shell; printed; grown); the materials, heights
  and spans are A-03's, scaled by them.
* Every variable acts at a scale the picture shows and the unit checks it reaches the drawing in every age it says it acts in. The
  colours live in the people's four banks: the structure (the ground's rock tinted toward the base, or painted; its brick, concrete,
  composite), the accent where the people puts it, the roofing or the glass in its tint, timber and rust where there is no light (the
  far future's structures alone glow: a dead city is dark).
* The relation to the land acts on each settlement until a plan of the world sites it (T-01); the street patterns pick C-01's plans
  until T-02 lays real ones.
* The proof: the sheet (`genome sheet`, a page an age, the twelve peoples nearest home), a people in every age (`genome ages`), and the
  measure (`genome distinct`): of the hundred nearest home no pair under a quarter of the strongest variables' weight, the nearest pair
  drawn side by side reading as two peoples.

## 4. The forms the renderer needs (A-02)

The ruins are boxes and domes, at three levels of detail, drawn within about 5 km. The ages need:
cylinders, cones and frusta; vaults and arches (round, pointed, barrel); domes of several profiles
(saucer, hemisphere, pointed, onion); pyramids and stepped masses; spires and needles; shells (saddles,
paraboloids); trusses and lattices; cables and catenaries; rings and tori; profiles extruded along a path
(a curved wall, a viaduct) and turned about an axis (a cooling tower); grown, blobby forms; the broken
versions of each for the decay (a tower snapped, a dome caved, a frame bent). Windows as a pattern on a
face (by the face's coordinates, like the grain), not as geometry. Colliders for each form (B-405's
heights). Levels of detail down to a silhouette, and a skyline band at the horizon, so a modern city or
a far-future spike is seen from thirty or fifty kilometres. Budgets: a city frame in `bench check`, the
threading contract, `stability` on a skyline.

**Done (2026-10-09, A-02):** `galaxy/forms.*` and `surface/forms_draw.cpp` (`docs/reference/12-civilisations.md`, "The forms
the renderer needs").
* Every form of the list, each with its broken state; the turned forms, the sweeps and the grids take numbers, so A-03 can ask
  for shapes beyond the sheet's samples. C-01's boxes and domes keep their arithmetic, so the C series' settlements draw to the
  pixel as they did.
* Windows are a pattern on the face's own metres, fading to their mean where a bay spans under two pixels.
* A level of detail a building (350 m, 1.3 km, 4 km), then a silhouette where it stands a pixel high: the skyline. The forms
  take the terrain's fog, so the air decides how far a city is seen: a ghost at 30 km on the peoples' hazy worlds, clear on an
  airless one (KI-373: a haze thinning with height is V-03's).
* Colliders for every form; a people's palette in banks 18-20 and 24 (the second people's 25-28).
* Budgets: the bench's city at 2x 8.3 ms of 16; `stability skyline` steady; the threaded frame the serial loop's.

## 5. A people's map of its world (T-01)

Today nothing is planned at the world's scale: each cell is hashed alone. The plan reads the world once,
coarsely (the planet function at tens of kilometres: land, height, slope, water and coasts, rivers,
climate, the dead seas, the divide between two peoples), scores where each age would settle (water and
flat land always; hills for defence in the medieval; coal and ore, read from the rock materials, and
rivers in the industrial; coasts in the modern; anything in the far future), and lays down a hierarchy
(a capital, cities, towns, villages, hamlets, spaced as each age travelled) and its networks (paths,
roads, rail and canals, highways, maglev) over as much of the world as the people held. The cells then
read the plan: a city spans many cells, a road runs between real places, a village sits where the
plan says. From orbit and on the maps the footprints show (the consistency principle: what is seen from
orbit is what is walked on): a modern people's grids on the landing map, a far-future people's works on
the globe. Built once a world on a thread at the approach, cached.

**Done (2026-10-09, T-01):** `galaxy/plan.*`, the cells' reading in `galaxy/ruins.*` (`docs/reference/12-civilisations.md`, "A people's
map of its world").
* The reading at half a degree (165,018 cells), the scores by age with the genome's relation to the land, the territory to the profile's
  held share, the hierarchy by rank with its founding order, population and drawn core, the networks of each age; 0.24-0.67 s on one
  thread a world, on a thread of its own at the approach.
* The cells read it: a place's cell holds it, a city's core spans cells and is drawn whole (to 24 km for the skyline), the countryside is
  the cells' own by the plan's density and stands only in the land a people held; C-09's lanes join the countryside, the plan's ways the
  places, laid on the ground by their kind.
* What is seen from orbit: the zoom and the telescope draw the cores and the ways at their true size; a modern people's grids and a far
  future's works on the globe are not drawn at the globe's scale, where a core is a thousandth of a texel (KI-376); the cities are their
  cores until T-02's districts.

## 6. Towns and works (T-02, T-03)

**Towns by age:** camps and rings; walled towns round a castle or a temple with fields outside; industrial
grids round the mills and the station, terraced streets climbing the hill; modern grids with a centre of
towers, a ring road, suburbs thinning into fields; arcologies and domes; the far future's absence of
towns. Districts (a centre, a harbour, the works, the houses, the temple, the graves), height and density
falling from the centre, an edge (a wall, suburbs, nothing). C-08's reading of the site by age: a quay, a
dock, a container port; terraces, fields, irrigation circles; cisterns, reservoirs.

**Done (2026-10-09, T-02):** `galaxy/towns.*` and `layoutTown` in `galaxy/ruins.cpp`, the ground's marks in `surface/site.*`
(`docs/reference/12-civilisations.md`, "Towns by age").
* Streets first, in the people's pattern and each age's way (a medieval town's crooked and narrow, an industrial quarter's straight with
  back lanes, a modern city's blocks of a hundred metres, its ring road and the suburbs' cul-de-sacs), then the buildings fronting them by
  district: the centre's square, the old core, the houses (densest at the centre and on the main streets), the works downwind, the harbour,
  a modern city's towers round its old core and its suburbs thinning into the fields; the edge a medieval wall with its gates, a ring road,
  nothing.
* The old cores and layers by H-02's eras: a city in rings of its ages, each in its age's way (a modern capital's medieval core, its
  industrial ring, its modern rim), the old wall's line a boulevard, a place left early in its older way and weathered from then, the old
  stones in a capital's fields; the countryside's villages and towns older by the history's shares, its lone monuments megaliths.
* C-08's reading of the site by age: a quay, a dock with its cranes, a container port with its gantries and containers; terraces; cisterns
  and reservoirs; fields (plots, strips, enclosures, irrigation circles) drawn into the ground with the streets and squares.
* No building across a street, the streets walkable, an old core older than its city: the unit, over 41 towns. KI-378 holds what stays.

**Works, "virtually anything":** quarries and mines, dams and reservoirs, canals and aqueducts, bridges,
observatories, shrines and pilgrims' roads, necropolises (C-12's graves widened: barrows, tombs,
cemeteries, mausolea), geoglyphs seen from the air, megaliths and henges, lines of fortification,
battlefields (between two peoples, C-13), launch sites and towers, antenna fields, wrecks, colossi,
terraformed land, the last shelter, the far future's inexplicable. Each is placed by a rule of the
terrain and the history: a dam at a valley's narrows, a quarry in the rock the town is built of, an
observatory on a summit, a launch site near the equator, a geoglyph on a flat plain, a necropolis across
the river. The scanner hears them (R-408) and the explorer marks them.

**Done (2026-10-09, T-03):** `galaxy/works.*`, the ground's edits in `surface/site.*` (`docs/reference/12-civilisations.md`, "The works").
* Twenty-one kinds, each where its rule or its history puts it: quarries in the rock of the stone-built towns (a face, a stone pit, an open
  pit; flint pits), mines on the plan's coal and ore, dams at a valley's narrows near the river towns that built them (up their rivers),
  holding their reservoirs, aqueducts from a spring on higher ground, observatories on the summits, a shrine at the holy place and the
  pilgrims' way to it, necropolises on the slope facing the city, lines and figures on a flat dry plain, henges and rows of standing stones,
  a fortified line on the frontier, a battlefield of each war's greatest battle between its two sides, launch sites near the equator, antenna
  fields, wrecks on the shore, a colossus over the capital, terraformed basins, the last shelter by the last place, wind farms, lighthouses,
  and the far future's: a plain made flat, pillars to the horizon, a mountain cut into terraces, a lattice over a valley, a sphere on its
  point.
* Each built in the way of the age that made it and weathered from when it was left; their ground drawn into the terrain (cuts, pits, ditches,
  craters, a reservoir's water or its old bed, lines, flats, terraces); heard by the scanner's WORKS mode, marked by the explorer.
* No bridges or locks yet, no works on the maps, a dam's search with the rivers off: KI-379.

## 7. Where the records lie (T-04)

By age and place: painted caves and rock shelters, stone circles, barrows; scriptoria, libraries,
temples, halls, houses, graves; archives, newspaper offices, stations, observatories; broadcast
stations, data centres, homes, bunkers; data vaults, an arcology's core; in the far future one archive
that keeps itself at the heart of a structure. The objects by age: carved stone and bone, a painted wall
(a chart on a cave's ceiling), clay tablets, scrolls, codices, wax cylinders, photographic plates,
printed pages, reels, discs, drives, crystals, something that hums. Archives hold several (a rich find);
the last recording lies somewhere that means it (the capital's mast, the last shelter, the temple's
inner room). C-03's rule stays: a copy taken is gone everywhere on the world.

**Done (2026-10-09, T-04):** `galaxy/records.*` (`docs/reference/12-civilisations.md`, "Where the records lie").
* 80 to 200 records a people by its age and its numbers (86 to 178 on the age worlds), C-02's fifty the first; each dated in its history
  (the later years the likelier), made in its year's way (carved bone, carved stone and a painted wall; clay tablets, scrolls, codices, an
  engraved plate; printed pages, wax cylinders, glass plates; reels, discs, drives; crystals; something that hums) and drawn as that object.
* Each with a home among its people's places that always holds it, copies anywhere else; a settlement's few in the buildings its age kept
  writings in, an archive's row in the capital's and every city's (some towns'), the far future's in rings round its structures, a few in
  the works that kept records; the last recording at the history's last place and in its last shelter; nothing written after a place was
  left.
* The shards found before kept by their index (the second people's moved to its new range), said once on the first load.
* The texts are still C-02's (H-04), the categories past the fifty at the fifty's shares (H-05): KI-380.

## 8. The histories (H-02)

The depth the user asks for ("dwarf-fortress level") is mostly not in the grammar: Dwarf Fortress and
Caves of Qud simulate a history first and then tell it. So a people gets a history, deterministic from
its profile, its places and its world: eras (one per age passed, named in the people's own way); the
founding and growth (the capital first, the cities in the plan's order, some abandoned early and older
in style); figures (rulers, prophets, makers, the last of them); institutions that change (chiefs,
kings, councils, a church, a republic, companies, a machine; gods and rites, C-02's god among them;
hunting, farming, trade, industry, information; what they valued: war, piety, trade, learning, art);
events (wars between cities or with the other people, plagues, famines, migrations, discoveries that move
the age on; disasters read from the world: an impact on a bombarded world, eruptions on a tectonic one,
the drying of a dead desert, a red dwarf's flares, an eccentric orbit's long winters, a comet's return);
and the collapse, its stages and its end.

**Done (2026-10-09, H-02):** `galaxy/history.*` (`docs/reference/12-civilisations.md`, "The history").
* A year at a time from the capital's founding to the end, from the profile, the plan and the world, in a few milliseconds: the eras
  with the people's names for them and their governments, faiths and livings; the plan's places founded in its order in the era whose
  ground suited them, a few left early, all left by the end in the cause's order; the rulers by their form (houses, heirs, regents,
  terms), the figures, ten families followed through the last 600 years; wars, sicknesses, harvests and famines, the world's own
  disasters, works, ways, inventions, discoveries, faiths, institutions, omens; two peoples' meetings, treaties, wars and conquest the
  same in both histories; the collapse in its stages to the last person at the last place; every cause kept.
* Not read by the game yet (T-02's old cores, T-03's battlefields and works, T-04's last place, H-04's records: KI-377).

**Causes of the collapse** by age, weighted by the world: prehistoric (famine, an ice age, a volcanic
winter, a flood, sickness, war between bands, a leaving); medieval (a great sickness, conquest, crop
failure, the drying, the cold centuries, civil war, an impact); industrial (poisoned air and rivers, coal
running out, industrial war, revolution, epidemics in the crowded cities); modern (a nuclear exchange, an
engineered plague, the climate, the food systems failing, cascading failure, a war for water, fewer and
fewer children); futuristic (a machine's decision, a nanotech accident, geoengineering gone wrong, a
stellar flare, a war with weapons that end worlds, a retreat into the machines, an exodus); far future (a
going elsewhere, an experiment with physics, an ending chosen, something the records cannot describe).

## 9. The records (H-03, H-04)

**Genres by age:** prehistoric (marks read as counts and pictures: the hunt, the herd, the dead, the
moon's turns; songs); medieval (annals, chronicles, letters, prayers and hymns, laws and decrees, ballads,
accounts and inventories, wills, epitaphs, a pilgrim's guide, a herbal, a sermon); industrial (newspapers,
letters, diaries, ledgers, scientific reports, patents, sermons, telegrams, factory rules, census
returns, timetables, obituaries); modern (news and weather broadcasts, messages, reports, logs, manuals,
minutes, propaganda, papers, diaries, emergency broadcasts, lyrics); futuristic (machine logs, personal
archives, a machine's notes, mission records, a charter, medical logs, codes of law, memorials, the last
messages); far future (records of things the translation cannot hold, instructions, mathematics, a
message to whoever comes).

**Length:** from a line (a tablet's count) to several paragraphs (a chronicle's year, a letter, a
broadcast's transcript), 30 to 400 words by the genre; the decoder pages them.

**The inputs** (the dozens the grammar reads): the world (its type, climate, seasons, day and year, sky:
moons, rings, a second sun, the band, comets, aurorae; its land's traits, its water, crops, animals and
weather; the place the record lies and what is near it); the people (age, name, the genome's meanings:
the sacred number, the focal monument; government, faith, economy, values); the history (the era, the
events round the record's date, the figures alive, the places standing, the war or the plague of its
time, the stage of the collapse); the record (genre, medium, author: a role, an age, a mood; the one it
is addressed to; the date in their calendar; the people's own metaphors, drawn from their world: "as
patient as the second moon"); the other people and how they stood; the language (C-06).

**Perspectives:** one event in an annal, a letter and a song; the official and the private disagreeing;
the proclamation and what the diary says. **The collapse from many sides:** the warning, the denial, the
last orders, the letter home, the inventory of what is left, the message to whoever comes. **The
register** (the ideas note's, section 1): the game is quiet; most records are ordinary (a harvest, a
child, a debt, a festival, the weather), and the catastrophe, late in the timeline, lands harder for it.

**Checks by construction and by the harness:** agreement (C-02's first print had "him mother", "the the
well", "a lizards"); no anachronism (no radio in a medieval text); no event before its cause or after
the end; every named place on the plan and every named figure alive at the date; variety measured
(distinct sentences, rule reuse) against C-02's; and reading fifty before judging.

**Done (2026-10-09, H-03), the engine:** `galaxy/grammar.*` and `src/galaxy/grammar/*.vg` (`docs/reference/12-civilisations.md`, "The
grammar engine"): the format, the link, the realiser (agreement by construction: C-02's "him mother" and "a lizards" cannot be written),
the checks (agreement, doubled words, length, an age's lexicon) and the measures; C-02's kinds rebuilt on it, every check at zero over
3,479 texts and 444 captions of 48 peoples. The genres, the inputs, the perspectives and the variety are H-04's.

**Done (2026-10-10, H-04), the records:** `galaxy/genres.*` and the genres in `src/galaxy/grammar/` (`docs/reference/12-civilisations.md`,
"The records (H-04)"): 63 genres by age and medium, the history read by the record model and told by its directives (one event from
several sides by salience), the collapse's witnesses in its years only, every place and figure the history's at the record's year (unit),
the lexicon by invention, the language by roots, the decoder's pages and the voice's opening. Over 48 peoples the repeated frames are 10.6%
at a people's first 50 texts and 34.6% at 100 (C-02's 20.4% and 36.4%); the records were read fifty per age and mended (KI-382 what stays).

## 10. The three categories, by age (H-05)

**More shards:** the catalogue grows from fifty to between about 80 and 200 a people, by its age and its
size (the literate and the many left more), in three categories: texts, music, star charts. The language
is still learnt from the shards held (C-06's share re-tuned for the larger vocabulary).

**Music by age** (C-04's tradition gains the age as an axis, the synth new families of timbre):
prehistoric (voices, skin and log drums, bone flutes, rattles; chants and dances in cycles); medieval
(plucked and bowed strings, reed pipes, bells, drones; modes, hymns, dances, laments); industrial
(ensembles of brass, strings and keys, a mechanical organ, a music box; heard through a cylinder's
crackle and a horn's narrow band); modern (electric strings and keys, drum machines, synthesisers; songs
with verses; tape hiss, broadcast compression); futuristic (algorithmic, microtonal swarms, synthetic
voices); far future (a field of sound rather than a piece). A record with no sound of its own (notation
on a tablet or a page) is played by the ship as the computer reconstructs it.

**Charts by age:** a sky painted on a cave's ceiling or carved on a bone, figures as animals, the moon's
turns counted beside them; an astrolabe's plate or a page, figures as creatures and heroes, the
wanderers' paths; an engraved atlas with a grid of their coordinates and catalogue numbers; a
photographic survey with magnitudes and the peoples heard by radio marked; a navigation chart of routes
between stars and the worlds they reached or meant to; the far future's chart of things the explorer
cannot find, or finds much later.

**Texts by age:** the media of section 7; a text without a voice of its own is read by the ship in the
reconstructed tongue (C-05's voice, flatter), a recorded one (from the industrial age on) is the voice
itself, through its medium. **On the air by age** (built with C-16, right after A-01, so the receiver is
designed once): spark-gap Morse and a phonograph broadcast; voices, music, numbers, a carrier's buzz;
data, synthetic voices, beacons; a beacon that is mathematics.

**Done (2026-10-09, C-16), on the air by age** (`programmeOf` by the slot's people's age; `12-civilisations.md`, "What the gain
lets in"):
* the industrial age: a phonograph in six text slots in ten (the voice through a horn over the record's surface) and in a
  third of the music slots (the piece), and otherwise Morse by spark (the text in a code of the people's own);
* the modern age: C-07's programmes;
* the futuristic age: a synthetic voice over half the text slots, then data and beacons, and the piece in one music slot in
  five;
* the far future: a beacon of mathematics (the primes, the squares, Fibonacci's numbers, the powers of two, the triangular
  numbers, the harmonic series, a count in binary, the digits of pi, by the world), and a synthetic voice in one text slot
  in four.

Each age has its carrier: the spark's rasp, C-07's tone, a thin whine, a pure tone at 1,420 Hz. The records of H-04 and H-05
reach the air through the slots without coming back to the receiver.

**Done (2026-10-10, H-05), the three categories by age** (`docs/reference/12-civilisations.md`, "The three categories by age (H-05)"):
* past C-02's fifty a record is music or a star chart by the age of its year (music 36% of the prehistoric records to 22% of the
  medieval, charts 6% of the modern to 15% of the far future's, leaned by the people's own fifty); every shard knows its age and medium;
* the music: C-04's tradition keeps its core (the scale, the cycle, the people's timbres) and gains an ensemble for each age (thirty
  families of timbre: voices, bone flutes, log drums and rattles; strings, reeds, bells, choirs and drones; brass bands, barrel organs,
  music boxes, keys and singers; drum machines, synth basses, electric keys and guitars; synthetic voices, swarms, struck glass; the far
  future's fields), forty forms in each age's texture (a caller and chorus, organum, chords stacked from the people's own scale, songs of
  verse and chorus, phasing processes, fields of long tones), 15 to 120 s, a limiter on the music's bus;
* the charts: twelve ways from a cave's ceiling, a carved bone and a painted vault through an astrolabe's plate, a codex page, an engraved
  atlas and a photograph on glass to a survey, a navigation chart of routes and the far future's chart of places where nothing is, each
  from the world's own data (the calendar's moon, the spin axis, the orbit and its wanderers, the voices heard); the far future marks a
  real star thousands of light years off;
* the media: a recording is heard through what it was made on (a wax cylinder's horn and crackle, a disc, a reel's hiss and wow, a
  drive's rot, a crystal, the hum), a record with no sound of its own is read by the ship in the people's voice made flatter, its
  notation played dry; the decoder names the medium and draws the object.

**Done (2026-10-10, H-06), the decoder reads a history** (`docs/reference/12-civilisations.md`, "The decoder reads a history (H-06)"):
a world's records on three pages: the records by kind, the timeline of the people's eras with the records held on it (an era placed and
named once a record of it is read), and the history as the records read so far tell it: the age they reached once three of its records
are read or the last recording (A-01's rule above), the people's ways the records expressed, the events told in order under their eras,
the persons only as the records named them, every line read through the language as the records are, the last recording closing it.

## 11. The research the grammar needs (H-01)

The user's words: "investigation on free-context grammar is required". A context-free grammar gives
variety but no memory: it cannot make a verb agree with its subject, keep one pronoun for one person, or
avoid telling the same event twice. The research settles what Vesperis needs on top of it.

**Questions:** (1) a plain weighted grammar, an attribute grammar (features passed down and up the tree:
number, person, tense, case, definiteness), or a grammar for the content over a separate surface
realiser; (2) where the depth comes from, the grammar or the history it reads (the hypothesis: both, the
history first); (3) how a long text holds together (genre schemas: a letter is a greeting, news,
feelings, a request, a farewell; a chronicle's year a date, an event, its cause, its consequence, an
omen) and how salience picks the events to tell; (4) the authoring format (a grammar language in text
files embedded into the binary at build time, by both build systems, or tables in C++) and a harness that
reloads it for fast iteration; (5) determinism and cost (a text in well under a millisecond, a world's
catalogue on a thread); (6) evaluation (automatic checks, variety measures, the reading); (7) the
language layer (the tongue, the voice and the radar's programmes on longer texts).

**Sources to read:** Compton, Kybartas and Mateas, *Tracery: an author-focused generative text tool*
(2015), and Compton's *So you want to build a generator* (2016); Grinblat and Bucklew, *Subverting
historical cause and effect: generation of mythic biographies in Caves of Qud* (2017); Tarn Adams on
Dwarf Fortress's world generation and legends, and *Procedural Generation in Game Design* (Short and
Adams, eds., 2017); Ryan et al., *Expressionist* (2016) and Ryan's *Curating Simulated Storyworlds*
(2018); Reiter and Dale, *Building Natural Language Generation Systems* (2000); Gatt and Reiter,
*SimpleNLG* (2009); Knuth, *Semantics of context-free languages* (1968, attribute grammars); Ranta,
*Grammatical Framework* (2011); Bruno Dias's *Improv* and his writing on *Voyageur*; Emily Short on
procedural text and salience-based narrative; Mark R. Johnson's *Ultima Ratio Regum*; Rumelhart's story
grammars (1975) and Propp's morphology (1928) for structure. Markov chains are set aside (no structure,
no control).

**What comes out:** `docs/ideas/RESEARCH-records.md` (the survey, the comparison, the choice and why), a
prototype in a scratch build printing texts of three genres for two worlds for the user to read, the
decision in `10-decisions.md`; the prototype removed from the tree once decided (the project's rule).

**Done (2026-10-08, H-01):** the history first, told by a small attribute grammar over a realiser, in `.vg` text files
embedded as raw strings and reloaded by the harness; the frequent parts composed (measured: the repeated sentence frames of
100 chronicles from 77% to 30%). The data model for A-01, H-02 and T-04 is the note's section 6; the prototype's fifty
records beside C-02's are in `RESEARCH-records-samples.md`.

## 12. Costs and risks

* **Size.** The largest work since Terrain III (O6): fourteen milestones, several of them L or XL.
* **A generation change** (13, one for the whole expansion, from A-01): every people's world changes
  (ages, ruins, shards, transmitters). Saves load; the shards already found are keyed by index, so the
  guide's found, decoded, heard and lent sets need a decision (kept by index, the plan's default, or
  cleared); the test worlds and the civilisation scenes are found anew, one per age near home.
* **The radar** (C-07, and C-15's tuning) changes with who transmits.
* **The picture.** Cities of towers seen from fifty kilometres on a CPU rasteriser at 2x-4x: LOD, the
  skyline band and the bench decide what is possible; the parked V series and the GPU question
  (`READING-terrain-and-lod.md`) touch the same budget.
* **Authoring volume.** Hundreds of grammar rules and the reading they need; the research must choose a
  format that makes writing and judging fast.
* **The synth** needs new timbre families for the ages' music.

## Left for later

Peoples that reached other stars (a far-future people's colonies with the same tongue and architecture,
their routes on the galaxy chart); a people's own maps of their world as a fourth category; the
futuristic and far-future peoples' outposts on the other bodies of their system (decided 2026-10-08:
later, the peoples stay on felisian and desert worlds in 1.3.0).
