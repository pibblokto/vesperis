# The unit's checks

Written by `tools/unitchecks.py` from `UNIT_GROUPS` (`src/harness/unit.cpp`) and its groups' functions: run it again
rather than edit this page (`make check` compares). `vesperis_test unit list` names the groups;
`vesperis_test unit <group> -q` runs one and prints its failures. A check in a loop is one line here.

42 groups, 401 checks in their order; a group's heading gives its file, its function, the render scale it
starts at and its count.

## noise (`unit_basics.cpp`, `unitNoise`, 1x): 9

- gnoise3d value equals gnoise3
- gnoise3d gradient by finite differences
- B-404 materials agree across the rings
- B-404 materials agree across the rings
- relief deterministic and bounded
- relief slope tracks finite differences
- material palette per type
- noon exposure rule
- material ramp hits c at 47

## kinds (`unit_kinds.cpp`, `unitKinds`, 1x): 11

- kinds: 1.3.2's trait eligibility kept
- kinds: the traits' classes are 1.3.2's split
- kinds: names safe, short names in ten characters, a ground for every landable kind
- made kinds: from their sources' draws, never a people's world, the same twice (P-01)
- made kinds: their share of the bodies (P-01's 1.4%, with P-02's living worlds 4.5%, with P-03's locked and deep 6.9%, with P-04's dead worlds 7.2%, with P-05's far cold 9.7%, with P-06's strange moons 11.3%, with P-07's giants 12.7%)
- made kinds: the home world felisian, Heilya II (P-01 to P-04)
- living kinds: from felisian (or thin air) worlds whose day is not their year, round living stars; a young star's alike (P-02)
- living grounds: each kind's signature on its first world (P-02)
- locked and deep kinds: from their sources, the locked ones locked and untilted, none half of a double planet (P-03)
- locked and deep kinds: the regress's and the bench's test worlds as drawn (P-03)
- locked and deep grounds: each kind's signature on its first world (P-03)

## dead (`unit_kinds.cpp`, `unitDead`, 1x): 3

- dead kinds: the scorched from a giant's felisian worlds and a white dwarf's rocky and desert planets, never C-01's people's; the frozen only as a wanderer, with no people (P-04, P-05)
- dead kinds: the ground the same in both times at every place of a scorched test world's and a staged frozen world's peoples; a frozen sea water to the placement (P-04)
- dead kinds: the scorched test worlds' peoples of their ages, built as their time's and weathered as the present's, ended as a desert's; C-01's first on the air (P-04)

## cold (`unit_kinds.cpp`, `unitCold`, 1x): 4

- far cold kinds: from their sources, a captured moon only the cryovolcanic, no air round a black hole or in a Wolf-Rayet wind (P-05)
- far cold kinds: wanderers in a few systems in a hundred, each its system's last body, far out on an eccentric, steep or retrograde orbit, no moon, no people, one in five a frozen felisian (P-05)
- far cold kinds: the regress's and home's cold worlds as drawn (P-05)
- far cold grounds: each kind's signature on its first world (P-05)

## moons (`unit_kinds.cpp`, `unitMoons`, 1x): 4

- strange moons: from their sources: a giant's large moons of ice, its outer ones facing it, its inner ones; moons in a resonance, small tumbling ones (P-06)
- strange moons: a two-faced moon keeps its apex, its dark face's middle, forward along its orbit (P-06)
- strange moons: the regress's and home's moons as drawn (P-06)
- strange moons' grounds: each kind's signature on its first moon (P-06)

## giants (`unit_kinds.cpp`, `unitGiants`, 1x): 4

- giants: from their sources, never a double planet's half, no people's moon moved, a hot giant round a yellow star in a hundred (P-07)
- giants: each class's air on its first giant (P-07)
- giants: each class's globe (P-07)
- giants: the probe's, the regress's and home's worlds as drawn (P-07)

## traits (`unit_kinds.cpp`, `unitTraits`, 1x): 14

- traits: the first three slots as P-07 drew them over 2,000 worlds (Q-01)
- traits: the new slots of distinct kinds, eligible, none excluded or moving a people's ground, packed; a fifth with none (Q-01)
- facts: as the orbit and the spin say, each seen (Q-01)
- marks: a sight's kind through the guide's file, R-408's numbers kept (Q-01)
- traits: an acidic world's karst towers stand (Q-01)
- traits: each new landform marks its ground and the scanner hears its sight there (Q-01)
- qualities: each carried, on a world it fits (Q-02)
- tides: their pulls' forcing, the range met at the equator the trait's, none at a pole, their flats wet sand under the sea (Q-02)
- qualities: the waters' colours, a milky sea's patches, a bloom's streaks, a reef's lagoon shallow, a pink lake (Q-02)
- qualities: the sky's by the hour, a fog sea at dawn and none at noon, a great aurorae's night at 25 degrees (Q-02)
- qualities: the ground's, a stratum's accent, the gypsum's dunes, a petrified forest's trunks, a spring's pool and its terraces, a strewn field heard (Q-02)
- sounds: each carried, on a world it fits, the dunes' slip faces and the ice there (Q-03)
- sounds: the ground's within their levels, the dunes at their note, the ice oftener at dusk, a quake through the boots muffled (Q-03)
- sounds: each cue where its world has it and nowhere else (Q-03)

## drifters (`unit_drifters.cpp`, `unitDrifters`, 1x): 18

- drifters: none in a sector with a star, each its key's place, an empty sector none (D-01)
- drifters: the field the same asked twice and nearer, its nearest forty as D-01 made them (D-01)
- drifters: no light, no season and the sun at the nadir on their worlds (D-01)
- drifters: nothing undefined from the anchor: positions, the almanac, the arrival, the parking (D-01)
- drifters: a save at one and the next as the target come back as they were (D-01)
- drifters: heard by their own law, never at x1, the slices the whole scan's (D-02)
- radar: a drifter heard, locked, plotted on the chart and flown to, its place named (D-02)
- drifters: a black disc where the stars stop, the survey's maps (D-02)
- radar: the drifters' voices, no two alike (D-03)
- radar: a drifter's voice in the game: the radar, the cabin's radio, the suit's (D-03)
- dark: the drifters' kinds by the dark's table (D-04)
- dark: the drifters' own traits, frozen seas (D-04)
- dark: the thermometer and the air on a drifter's ground (D-04)
- surface: the explorer's lamp by night, nothing by day (D-04)
- drifters' peoples: one frozen felisian or desert drifter in four, each its former star found, the loss of the sun its cause in its order, two ending together (D-05)
- history: a drifter's people's the same from the decoder and from the ground (D-05)
- works: a people that lost its sun's works of the end, its last recording in the last shelter alone (D-05)
- radar: a drifter's beacon, a radio people's last broadcast under its voice, heard (D-05)

## basics (`unit_basics.cpp`, `unitBasics`, 4x): 74

- lat/lon round trip
- a landable planet found
- site local/unit round trip
- sun at the zenith over the sub-solar point
- sun at 60 deg 30 deg along the meridian
- sun due south/north from there
- local time at the sub-solar point is noon
- Kepler equation residual
- circular orbit is the identity
- gnoise3 mean near zero
- gnoise3 within [-1.1, 1.1]
- hash uniform (mean 0.5, var 1/12)
- noise is deterministic
- name lengths 3..16
- names are letters, spaces, apostrophes
- names deterministic
- buggy deploys at the capsule
- B replaces a buggy left far away
- get in
- nose camera pans 70 deg at most
- no buggy on a new landing
- reanchor keeps the waypoint in place
- a new landing clears the waypoint
- a thin-atmosphere world found
- a dust storm found
- storm wind under 200 knots
- seasonOf bounded by the tilt
- map cache honours its cap
- 32 palette banks of 2048 intensities
- body B's sand has its own bank
- a globe with its centre behind the camera still shows its limb
- a system with a companion star found
- the companion ahead is a disc of the expected size
- the companion 70 deg off axis leaves no disc
- the companion beside the camera (95 deg) does not flood
- the companion behind (150 deg) does not flood
- companions keep clear of the other sun's worlds (B-425)
- the reported giant: a small sun over the same worlds (B-425)
- a night under a far companion is a night (B-425)
- a companion's world keeps its own night (B-425)
- a companion's world keeps its own night (B-425)
- moons spread: a system with distance from the ship, discs still in the sky (B-428)
- a ringed world found
- a globe centred left of the frame shows its limb
- a globe centred right of the frame shows its limb
- the same globe in a sky shows its limb
- sky bodies lie beyond the far ring
- a small body close by lies beyond the far ring too
- the mountain review site set up
- (made by `(std::string("rings meet: ") + names[k]).c_str()`)
- a grassland herd site set up
- the landing's own herds are there
- habitat herds spawned within 900 m
- 3 km on, the herds behind are forgotten
- 3 km on, new herds are within reach
- every animal belongs to a listed herd
- a landable planet with a moon found
- the moon's shadow crosses the landing map
- sector of 0,0 is 180:090
- sector of the south-west corner is 000:000
- sector of the north-east corner is 359:179
- a ringed landable planet found
- ring profile bounded with a dense part
- the ring shadows the ground somewhere
- rangefinder 45 deg down hits within 1..6 m
- the far ring reaches 53 km
- comets are landable
- comet escape velocity under 30 m/s
- small body: terrain rings capped near the horizon
- no buggy on a comet
- sprinting on a comet stays under 0.3 escape velocity
- drone: lifts, 320 km/h, ceiling, lands, out (R-403)
- drone: a grassland site for the flight
- jetpack over and out of the water (B-406)

## signals (`unit_signals.cpp`, `unitSignals`, 4x): 9

- signals: hashed, rare and far (C-07)
- radar: the sweep hears, locks, flies (C-07)
- radar: the receiver through the synth (C-07)
- radar: the receiver's variety (C-07)
- signals: the gain, x1 to x32 (C-15)
- radar: the gain in the game (C-15)
- radar: what the gain lets in, the receiver (C-16)
- radar: each age on the air (C-16)
- radar: the top step in the game (C-16)

## forms (`civ_forms.cpp`, `formsChecks`, 4x): 4

- forms: every kind whole and broken (A-02)
- forms: walking into each one (A-02)
- forms: the peoples' banks (A-02)
- forms: the city frame is the serial loop's (A-02)

## ages (`civ_peoples.cpp`, `agesChecks`, 1x): 5

- the decay by material (A-03)
- what each age builds (A-03)
- what each age builds: its own kinds, its heights (A-03)
- every age's pieces rest on something, its rooms open (A-03)
- the decay's examples: post holes, a stump, a spire still sharp (A-03)

## genome (`civ_genome.cpp`, `genomeChecks`, 1x): 4

- the genome (A-04)
- the genome is deterministic (A-04)
- every variable reaches the drawing (A-04)
- no two of a hundred peoples alike (A-04)

## plan (`civ_plan.cpp`, `planChecks`, 1x): 7

- the world plan: the same twice and on the workers, within a second and a half on one thread (T-01)
- the world plan's places: none in the sea, the capital on the territory's best ground, the networks reaching every place (T-01)
- the cells read the plan: no settlement in the land no people held, none in a city's core but the city (T-01)
- a city's core spanning cells drawn whole: reached, gathered and walked into from a cell of its core not its own (T-01)
- the plan on the ship and the ground: the zoom shows a modern capital's core and its highways, the scanner hears it from 150 km, a way's legs on the ground (T-01)
- the landing map's peoples: off as it opens, the places of the records held, all of them the whole map (R-415)
- the landing map's cursor: no drainage tiles for the places it passes, the place it rests at's 0.4 s on (B-422)

## history (`civ_peoples.cpp`, `historyChecks`, 4x): 5

- H-02: a people's history the same twice, in milliseconds
- H-02: every event in the history's years, its figures alive at it, its places the plan's and standing
- H-02: the collapse's stages in order to the end, every place left by it, the last place at it
- H-02: causes before their effects, battles within their wars, the kin and the reigns coherent
- H-02: two peoples' meetings, treaties, wars and a conquest the same in both histories

## towns (`civ_plan.cpp`, `townChecks`, 4x): 5

- towns: no building across a street (T-02)
- towns: the streets walkable, the colliders off them and the web joined (T-02)
- towns: one age a world, a place left before it gone, one left early in it weathered longer (B-414)
- towns: the districts and works of each age (T-02)
- towns: a harbour of each age, the streets and fields in the ground (T-02)

## works (`civ_works.cpp`, `worksChecks`, 4x): 8

- works: a dam across a valley holding a reservoir (T-03)
- works: a quarry's face in the rock of a stone-built people, its pit under the ground (T-03, B-418)
- works: the land's, each inside its cell, none touching, clear of the settlements, the cores and the world's, the same built again (B-418)
- works: the plan, a people's extent, its settlements, its works and its land's the same in every season (B-419)
- works: the scanner hears one within 40 km of nine points in ten of a people's land and of its places (B-418)
- works: a battlefield between the two sides of its war (T-03)
- works: the scanner's WORKS mode hears them (T-03)
- works: the same twice, out of the cores, every kind laid, their forms sound (T-03)

## records (`civ_records.cpp`, `recordsChecks`, 4x): 7

- records: 80 to 200 a people by its age and numbers, dated in its history, each in its year's medium (T-04)
- records: no two copies in one place, none written after its place was left, an archive in every capital and city of a literate age (T-04)
- records: an archive its class's count, the place's own beyond it in its other buildings, every place holding its own (B-426)
- records: the last recording at the history's last place and in its last shelter, nowhere else (T-04)
- records: an old guide's shards kept by their index across the change (the second people's moved), the counts and the language kept, said once (T-04)
- records: a guide of the records before one age a world keeps its keys, its contents new, said once (B-414)
- records: each object drawn, offered and taken with E in a modern capital (T-04; `records scenes` the six ages)

## walls (`civ_walls.cpp`, `wallsChecks`, 1x): 4

- walls: every collider over what is drawn (B-420)
- walls: walked through the places, stopped by what is drawn alone (B-420)
- walls: a heap, a bank or a barrow stood on and walked over (B-420)
- records: none in water, in or under a piece, out of reach (B-421)

## grammar (`civ_grammar.cpp`, `grammarChecks`, 1x): 11

- grammar engine: pronouns where clear, verbs by tense and person, numbers with their nouns, plurals, a and an, attributes passed up, once, keep, guards, a directive, a path, a person brought in, I and you (H-03)
- grammar: the records' genres as .vg files, compiled in, linked without an error, the tree's copy the same, each under 16 KB (H-03, H-04)
- grammar: every .vg file in the tree compiled in (H-04's second pass: the harness reads a new one from disk before the build lists it)
- tongues: each people's names in its own tongue, a held-out name told its people from its letters, every name readable (H-04's second pass)
- history: the everyday of the quiet years and of the families, the twenty-one kinds over the test worlds' peoples, each at a place standing, its persons alive, the families' private (H-04's second pass)
- records: every check at zero over every record of the test worlds' peoples, every name the history's at its year (H-04: no anachronism, no unknown place or figure)
- records: the first failure
- records: the genres of every age told, one event from several sides, the cost (H-04: a text under 100 us)
- records of the lost sun: the test drifters' peoples, every check at zero, every name the history's, their own genres told (D-05)
- records of the lost sun: the first failure
- records: no other people's moved by the lost sun, the age and scene worlds' peoples as D-04 left them (D-05)

## media (`civ_records.cpp`, `mediaChecks`, 1x): 3

- media: the decoder's (H-05)
- media: on the decoder a record with no sound of its own is read or played by the ship, a recording heard through what it was made on (H-05)
- media: through the synth each medium keeps the level, its surface where it should be, the ship's reading flatter (H-05)

## pages (`civ_records.cpp`, `historyPageChecks`, 4x): 12

- history: the decoder's history page (H-06)
- history: the decoder's history page (H-06)
- (made by `fmt("history: the page holds what the read records told and nothing else, %s (H-06)", when).c_str()`)
- (made by `fmt("history: a person named only as a read record wrote them, the eras placed by the records read, the age %s, %s (H-06)", ageWanted ? "told" : "not told yet", when).c_str()`)
- history: Tab turns the decoder's pages to the history (H-06)
- history: a record read on the decoder adds what it told (H-06)
- history: Enter on an event opens the record that told it (H-06)
- history: a record read on the decoder adds what it told (H-06)
- history: the records page shows one kind (H-06)
- history: Shift held with an arrow runs the timeline, a tap one record, stopping at the ends; the arrow alone goes round (R-416)
- history: Shift held with an arrow runs the history's events, a tap one, stopping at the ends (R-416)
- history: Shift held with Up/Down runs the records page through the kind shown, stopping at the ends; Down alone goes round (R-417)

## instruments (`civ_instruments.cpp`, `instrumentChecks`, 4x): 4

- instruments: each age's, from its people's ensemble (C-11)
- instruments: beside the music records (C-11)
- instruments: their sound (C-11)
- instruments: one of each age found, taken and played (C-11)

## makers (`civ_instruments.cpp`, `makerChecks`, 1x): 3

- instruments: made by their peoples, of what their worlds gave (C-17)
- instruments: as many as their peoples (C-17)
- instruments: the decoder names them as the ruins do (C-17)

## player (`civ_instruments.cpp`, `playerChecks`, 1x): 6

- the player: through a flight to a landing (C-11)
- the player: through a flight to a landing (C-11)
- the player: lists the pieces, the recordings and what the radar caught (C-11)
- the player: held by the decoder's own record, taken up where it was (C-11)
- the player: a recording the radar caught plays as kept off the air (C-11)
- the player: plays on through a Vimana flight, stops at a landing (C-11)

## drones (`modes_drones.cpp`, `droneChecks`, 1x): 4

- the drones: a world's items (C-18)
- the drones: a world's items (C-18)
- the drones: the hauls (C-18)
- the drones: a world of two peoples, both in a trip (C-18)

## navigation (`unit_navigation.cpp`, `unitNavigation`, 1x): 18

- a belt found near the start
- rocks within two cells of the anchor
- a rock is the same rock two hours later
- the rock stays inside the belt
- rocks are found round a rock after the rings sheared
- a planet approached from a belt keeps its parking (B-410)
- a jump of 400 ly along the aim (R-407)
- a jump into the void comes back to the stars
- the aim's markers name no UNKNOWN, say VISITED
- the galaxy chart puts the ship where it is (G-05)
- three arrivals visited, no route drawn between them (R-414)
- the galaxy page in a Vimana flight (G-05)
- a jump plotted on the chart and flown (G-06)
- a plot into the halo comes back to the stars (G-06)
- a mark of the galaxy through the guide and an export (G-06)
- N steps through what the explorer has (G-06)
- a mark of the galaxy renamed and removed (G-06)
- the band in a Vimana flight (G-07)

## sights (`unit_navigation.cpp`, `unitSights`, 4x): 9

- the sights by kind: lakes are water, peaks the tops (R-408)
- a sight named before is a mark where it stands
- the scanner hears the ruins, the marked left out (R-408)
- the map's cursor renames, removes, waypoints; L marks
- the scanner's WORKS mode in the game, a work's mark kept (T-03, B-418)
- marks: none over the view until one is shown from the map, one at a time (B-412)
- marks: the shown one hidden again, kept through a removal and the guide (B-412)
- marks: the far ones read without building a drainage tile, with their rivers once built (B-422)
- the marks go through the guide and lend

## stars (`unit_sky.cpp`, `unitStars`, 4x): 22

- magnetic field: every class occurs
- storms on some nights, not most
- strong field, active star: most nights
- weak field: rarely
- star classes: red dwarfs the commonest
- star classes: every class near home
- red dwarf flares: several a day
- red dwarf: near worlds locked
- carbon star: carbon worlds
- neutron stars: some near home, none with a living or giant world
- neutron stars: a glassed survivor in most systems with planets
- glassed worlds: sheets over 15-85% of the land, the same at 512 m
- protostars: a trickle in the arm, half the yellow family in a complex
- protostars: belts and young worlds only
- protostar: its cloud glows round it
- Wolf-Rayet stars: rare in the arm, none in a globular
- Wolf-Rayet stars: bare worlds only, bombarded in most systems
- Wolf-Rayet star: the shell's ring round the sun
- a Wolf-Rayet giant's stripped tail: its own dim cloud, behind the globe by day, over it by night
- black holes: a few near home, several times as many in the core
- black holes: rocks, rubble and a close companion
- black hole: the star behind it becomes an Einstein ring, the shadow black

## civilisations (`unit_civ.cpp`, `unitCivilisations`, 4x): 10

- civilisations: one felisian or desert world in eight
- dead desert world: dry seas, the bed under the land, wadis to the shore
- dead desert world: dry seas
- drainage: a superseded prefetch's tiles abandoned, a tile waited for finished (B-422)
- settlements: hamlets, villages, towns and lone monuments
- settlements: every room open, its doorway a metre clear
- shards: a few per settlement, in a room or at a stela, clear of the pieces and the doorway, distinct, deterministic
- settlements: the colliders leave the rooms and doorways open and stand as tall as their walls
- shards: the town's sites are clear in the view, within reach at a metre, drawn, and gone once taken
- graves: the town's stones among its pieces, the first within reach at a metre, its name and years read (C-12)

## ruins (`unit_civ.cpp`, `unitRuins`, 4x): 2

- ruins read the world: harbours, terraces, cisterns, wind walls and cairns where the terrain says (C-08)
- weathering by age: the decay with the years and the rain, from standing to foundations (C-08)

## charts (`unit_civ.cpp`, `unitCharts`, 4x): 3

- star charts: a few of the fifty, the people's sky round a star they marked, figures named in the caption (C-10)
- star charts by age: each in the way of its record's age and medium, with what that way shows (H-05)
- star charts on the decoder: read, targeted, held up to the sky and put down (C-10)

## roads (`unit_civ.cpp`, `unitRoads`, 4x): 3

- roads: a network between the sited settlements, from edge to edge over dry ground, the same from either end (C-09)
- graves on the ground: the explorer before a stone is told once, the guide keeps it (C-12)
- roads on the ground and on the ship: the bed paved and bare, the tone from the air, the zoom's lines, the explorer told (C-09)

## graves (`unit_civ.cpp`, `unitGraves`, 4x): 2

- the last recording: one a world, a text of the latest year with the end's tone; every date within the calendar (C-12)
- graves: a few a settlement outside its edge, clear of the buildings, named and dated in the span, the same twice (C-12)

## cultures (`unit_peoples.cpp`, `unitCultures`, 4x): 5

- cultures per world: about one civilisation world in five had two peoples (C-13)
- a world of two peoples: two lores, tongues, voices, traditions and fifties, each naming the other where they met, one last each, the ends by the coin; one-people worlds as they were (C-13)
- a world of two peoples on the ground: the divide, each side's settlements its people's with shards of its fifty, the frontier's roads the bigger settlement's build (C-13)
- a world of two peoples on the decoder and the ground: an entry a people, the second's list its own, its last ends its own record; a shard taken counts against both peoples' records (C-13)
- lending: a friend's read shards travel with the inbox, listed and read in cyan, counted in the language, the explorer's own once taken (C-14)

## profiles (`unit_peoples.cpp`, `unitProfiles`, 4x): 3

- ages: a people's profile is one function, the same twice, its eras, radio, collapse and end in order (A-01)
- ages: the shares as decided, the far future rarest, about three in ten with radio; a test world of each age where pinned (A-01)
- ages: only the radio ages on the air: a transmitter's people had radio, a candidate without is silent; two peoples' slots, the charts' heard marks (A-01)

## story (`unit_peoples.cpp`, `unitStory`, 4x): 10

- records: deterministic, complete, each within its genre's length, the texts distinct (C-02, H-04)
- records: the world's and the history's names recur
- records: the register: mostly the ordinary life, the collapse's witnesses only in its years (H-04)
- decoding: a third of the words read with one shard, all with the people's full count, never fewer with more, the names from the start, the people's words the same wherever they recur
- music: a tradition a world, an ensemble an age (C-04, H-05)
- music: every piece at every age in its length, its notes on its ensemble (H-05)
- voice: a voice a world, every text's opening spoken in time with its words
- voice: a shard through the synth
- music: a piece of each age through the synth (C-04, H-05)
- shards: every people's world near home reads clean

## sky (`unit_sky.cpp`, `unitSky`, 4x): 39

- almanac: the three test places
- (made by `fmt("almanac: %s", tp.what).c_str()`)
- almanac: from the ship at the giant
- almanac: the run lands on the event
- almanac: the free row runs for an hour
- almanac: T stops a run
- telescope: x2 doubles the disc and tracks the world
- telescope: the stabiliser holds the world on the reticle
- telescope: the steps are powers of two
- telescope: the stabiliser holds the ground under the reticle and the ship over its ground
- telescope: the mouse steers at the rate of the magnification
- telescope: Enter lets go and the stabiliser still holds the view
- telescope: N names the target and the photo says so
- telescope: the sun on the reticle
- telescope: Z stows and the view is the setting's again
- telescope: stowed, the lap goes on
- telescope: Shift+Right carries the ship round the world, the world kept in the window
- telescope: Shift+Up carries the ship north
- telescope: opened after the turn, it holds the side faced
- telescope: the plate begins coarse within its first frame
- telescope: the plate is whole within two seconds and finer than the world map
- telescope: the plate's cells are the planet function's own numbers
- telescope: a zoom step begins a finer plate and keeps the last as the fallback
- telescope: P reads the plate out whole for the photo
- cabin: the telescope's set on the back wall, E looks through it
- cabin: every computer in a colour of its own
- cabin: a glass hull shows the outside, what stands in the room stays solid (B-411)
- cabin: the light works through a glass hull, and U says the hull is glass (B-411)
- sound: the ground's stay on the ground (B-413)
- fronts: a front reaches the pinned plain within ten days
- fronts: the forecast's arrival is the moment the rain begins
- fronts: the clearing is the moment the rain ends
- fronts: the almanac lists the arrival and the clearing
- fronts: the map's band and the ground agree
- fronts: the data sheet forecasts the front, the sector map draws its line
- showers: the peak is the orbits' crossing (W-04)
- showers: streaks from the radiant (W-04)
- zodiacal light: the cone and its dust (W-04)
- earthshine: a crescent's night side (W-04)

## probe (`unit_probe.cpp`, `unitProbe`, 1x): 15

- probe: the giant's air and the clock (X-01)
- probe: aimed by the telescope (X-01)
- probe: Shift runs the relay's clock x8 (R-406)
- probe: saved and resumed in the descent (X-01)
- probe: leaving cuts the relay; the end (X-01)
- probe: the clouds are the globe's bands (X-02)
- probe: the decks keep their order (X-02)
- probe: every giant its own decks and kind (X-03)
- probe: the great storm is the globe's (X-03)
- probe: no giant's picture black or clipped flat (B-409)
- probe: a recording and its profile kept, read back and lent (X-04)
- probe: the recording plays from the gallery as it fell (X-04)
- probe: two places of one giant, two profiles (X-04)
- probe: what it crosses goes to the log with its depth (X-04)
- probe: a storm renamed in its recording, then Esc to the gallery and to the ship (B-429)

## perf (`unit_proof.cpp`, `unitPerf`, 4x): 13

- threads: the frames the regress skips (F-01, B-423)
- threads: the picture whatever the count (F-06)
- guide: the probes' lines made once (F-02)
- png: the writer's thread writes the call's bytes (F-02)
- stars: the neighbourhood kept across moves (F-02)
- landing map and bay: no wait for the world's map (F-02)
- warm: a fresh site's rings filled on the pool (F-03)
- drainage: the two-cell pit sunk a binade at a time (F-03)
- terrain: the cells binned by their rows (F-04)
- feeds: the noise's rows from the jump (F-05)
- wrap2pi: a turn either side without fmod, the bits it had (F-07)
- ifloor: the truncation's floor and this build's, std::floor's integer (F-07)
- mush: the box's quotient by a multiply and a shift (F-07)

## auto (`unit_navigation.cpp`, `unitAuto`, 1x): 4

- auto-explore: begun from the menu, eased, given back (R-419)
- auto-explore's tape: M, the deck, credits, ducked, kept (R-420)
- auto-explore's tape: the cassette deck (R-420)
- auto-explore's tape: the explorer's folder, else the game's own (R-420)

## states (`unit_navigation.cpp`, `unitStates`, 1x): 1

- the states' table, the old lists
