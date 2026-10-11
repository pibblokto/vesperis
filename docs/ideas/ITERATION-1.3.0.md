# Iteration 1.3.0: the far, the faint and the peoples

Planned 2026-10-08 on the user's three requests, and set the same day as one release with everything in
it, in the order below (the user: "Write all of it into this release as well and structure everything
in the best order for implementation you see fit so I can just say proceed after that and you can kick
off").

**The far and the faint** (`R-409`, `R-410`): a chart of the whole galaxy that shows where the ship is
and how dense the stars are, with the next jump chosen on it, and a radar that can be turned up to hear
farther signals, at the price of radio noise and "creepy mixed sounds". 1.1.0 made the galaxy two
hundred billion stars and 1.2.0 let a jump cross it (R-407), but nothing in the game shows the galaxy's
shape (the star map is the ten light years round the ship) and the radar hears 200 ly and no farther.

**The peoples** (`R-411`, `IDEAS-civilisation-expansion.md`): the civilisation expansion in three blocks.
**A, the ages and what they built**: a people found at the age it had reached when it collapsed
(prehistoric, medieval, industrial, modern, futuristic, far future), each age shown in what it built,
how and how densely, and an architecture genome of dozens of variables that makes every people's
buildings its own. **T, the towns and the works**: settlements placed by a plan of the whole world,
towns of each age, and everything else people make, with the records' places in them. **H, the
histories and the records**: research into grammars first, then a simulated history per people, a
grammar engine with dozens of inputs, longer records of many genres, and more shards in three categories
(texts, music, star charts) styled by the age.

**Generation.** The galaxy chart and the crossing change nothing generated. The peoples are one
generation change (13, from A-01): every people's world changes, the regress is re-blessed where a
hashed frame shows one, and the scenes and test worlds of the civilisations are found again, one per age
near home. The radar's gain (C-15) comes after A-01 and is tuned on the transmitters A-01 leaves.

Left out on purpose: the V series (parked until the rendering direction is settled;
`READING-terrain-and-lod.md` first), W-02, W-05, S-07, the P, L and K series. They stay in
`MILESTONES.md` as they are.

## Order

| # | Phase | Id | Item | Size | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | The far | G-05 | The galaxy chart: the whole disc by its stars, where the ship is, home, the way it came, the signals it locked | M | done 2026-10-08 |
| 2 | The far | G-06 | The next jump chosen on the chart: a cursor, what lies there, the jump plotted and flown, a look before the leap | M | done 2026-10-08 |
| 3 | The far | G-07 | The crossing: the galaxy turning round the ship in a long flight | S | done 2026-10-08 |
| 4 | Groundwork | H-01 | Research: grammars and histories for generated records; a written choice and a prototype to read | M | done 2026-10-08 |
| 5 | Groundwork | A-01 | The ages: each people's age at its collapse, its profile, the radio ages alone on the air; generation 13 | M | done 2026-10-08 |
| 6 | The faint | C-15 | Deep listening: the radar's gain, a power per transmitter, the far scan, far signals faint and slow to lock | M | done 2026-10-08 |
| 7 | The faint | C-16 | What the gain lets in, and each age on the air: the floor, the galaxy's noise, signals mixed, far voices worn, voices in the static | L | done 2026-10-09 |
| 8 | What they built | A-02 | The forms the renderer needs: new shapes, their broken states, colliders, LOD to a skyline, a people's colours in banks | XL | done 2026-10-09 |
| 9 | What they built | A-03 | What each age builds: the buildings, materials, heights and density of six ages; decay by material | L | done 2026-10-09 |
| 10 | What they built | A-04 | The architecture genome: thirty-five deep variables and the age's own; the architecture sheet; distinctness measured | L | done 2026-10-09 |
| 11 | Where and when | T-01 | A people's map of its world: a plan of capital, cities, towns and villages by the age, networks, seen from orbit | XL | done 2026-10-09 |
| 12 | Where and when | H-02 | The history: a simulated history per people, its eras, figures, institutions, events, the collapse by its cause | L | done 2026-10-09 |
| 13 | Towns and works | T-02 | Towns by age: plans, districts, old cores by the history's eras, density and height, edges, the site's reading | L | done 2026-10-09 |
| 14 | Towns and works | T-03 | The works: everything else people make, placed by the terrain's rules and the history's events | L | done 2026-10-09 |
| 15 | Towns and works | T-04 | Where the records lie: places and objects by age, how many a people left, the found shards across the change | M | done 2026-10-09 |
| 16 | The records | H-03 | The grammar engine: H-01's formalism, the realiser, the authoring format, the checks; C-02's texts rebuilt on it | L | done 2026-10-09 |
| 17 | The records | H-04 | The records: genres by age, longer texts, dozens of inputs, the collapse from many sides | XL | done 2026-10-10 |
| 18 | The records | H-05 | More shards in three categories styled by the age: texts, music, star charts | L | done 2026-10-10 |
| 19 | The records | H-06 | The decoder reads a history: categories, long texts, the eras' timeline, the people's history as read | M | done 2026-10-10 |
| 20 | The player | C-11 | The ship's player and the instruments of the ages (KI-347) | M | done 2026-10-10 |
| 21 | The makers | C-17 | The instruments' maker: each people's instruments built from parts by its own way of making (added 2026-10-10) | L | done 2026-10-10 |
| 22 | The makers | C-18 | The drone bay: three drones with hands sent from the ship to a people's world for its records and instruments; the instruments' case by world (added 2026-10-11) | L | done 2026-10-11 |

Each row's `Status` moves to `done` (with the date) when its milestone is finished, and the matching row
of `MILESTONES.md` gets `1.3.0, done <date>`. The ids keep their series' names; this table is the order.

## Why this order

1. **The far first.** Three small, self-contained milestones with no generation change, playable at
   once; the chart is where the far signals and, later, the peoples' worlds are found.
2. **The research before any of the peoples' code.** It is reading and prototyping and waits on
   nothing, and its answer shapes the data every later milestone lays down (the profile, the history,
   the grammar's inputs).
3. **A-01, then the radar.** A-01 decides which peoples had radio. Tuned after it, the gain's powers
   and the transmitters' salt are pinned once, on the transmitters that will ship, and C-16 designs each
   age's sound on the air in the same pass as the noise (tuned before it, C-15 would be tuned twice and
   the programmes by age would come back to the receiver at H-05).
4. **The renderer's forms, the ages' buildings, the genome.** A-02 is the riskiest engine work (new
   shapes, a skyline seen from tens of kilometres, the CPU's budget), so it comes early enough for a
   problem to change the plan; A-03 and A-04 then put the ages and the uniqueness in front of the user in
   play while the rest is built. Until T-02 the new buildings stand in C-01's settlement plans.
5. **The plan, then the history.** T-01 lays down where the peoples lived; H-02 tells what happened there
   before the towns, the works and the records are made (Dwarf Fortress's order: the history before the
   sites' detail), so they can read it: a city's old core of its founding era, a battlefield where a war
   was fought, the last shelter of the collapse, the record that names them.
6. **Towns, works, the records' places.** They read the plan, the history, the buildings and the genome;
   T-04 fixes how many records a people left before the texts are written.
7. **The records.** The engine on the research's choice (H-03), the content over the history and the
   places (H-04), the three categories by age (H-05), the decoder that reads it all (H-06).
8. **C-11 last,** so its instruments are the ages' (H-05's music).

## Decided (2026-10-08)

The user handed over the order and the questions the plan had left open. The plan's defaults stand, and
each can be changed at any milestone's review; a milestone starts on "proceed" without asking them again
(a new design question that is the user's is still asked before building):

1. **One release:** everything above is 1.3.0.
2. **G-05:** the globulars, the open clusters and the star-forming regions are drawn before they are
   visited, unnamed, as the galaxy's shape.
3. **G-06:** the explorer's marks of the galaxy are in (`L` names the place under the cursor; kept and
   lent like R-408's marks).
4. **C-15:** six steps, x1 to x32; the strongest transmitters heard to about 6,000 ly; one to three
   peoples' signals added per step from home.
5. **A-01:** the user's six ages (no ancient age between the prehistoric and the medieval); the early
   ages commoner and the far future rare; the peoples stay on felisian and desert worlds (outposts on
   their systems' other bodies are left for later).
6. **A-01:** a people's age is shown, never named on a screen, until its records tell it (the decoder's
   history page, H-06).
7. **T-04:** the shards found before generation 13 are kept by their index (the counts and the language
   kept, the contents new), with one status line on the first load.
8. **C-11 is in,** last.

## Phase 1: the far

Sources: `docs/iteration-3/plans/requests/R-409`, `PLAN-galaxy-scale.md` (the density, the regions, the
clusters), the R-407 and R-408 sections of `docs/reference/10-decisions.md`, KI-364.

### G-05: the galaxy chart

The star map shows the ten light years round the ship (`nb.stars`, rings at 2, 5 and 10 ly) by the
galaxy plan's design; the galaxy is 110,000 ly across and only the harness draws it
(`vesperis_test galaxy`, `galaxy_faceon.png`). The chart is a second page of the star map: `Tab`
turns the map between THE NEIGHBOURHOOD and THE GALAXY, and the map opens on the page it was left
on, so `M`, the right wall's star map screen and the guide menu's `STAR MAP` all reach it. The
galaxy page opens in a Vimana flight too, where `M` is refused today; the neighbourhood page stays
as it is.

* **The disc by its stars.** The face-on column density of `galaxyDensity`, log-toned as the
  harness's map, drawn in the light of its population: the core and the bulge warm (old stars), the
  arms blue-white, the disc between them dimmer, the halo nearly black with the 145 globulars as
  fuzzy points; at the closer zooms the open clusters as bright knots and the star-forming
  complexes as faint red patches, read from their cells (`openClusterInCell`, `nebulaInCell`)
  rather than from the column, which steps over anything 30 ly thick. The density is the galaxy's
  shape, like the band in the sky: nothing on it has a name, and the marks on it are the
  explorer's own (R-408's rule).
* **The edge-on strip** under the disc: its thickness along the chart's width and the ship's
  height over or under the plane (R-407's jump north can leave it 2,300 ly over).
* **Zoom and pan.** From the whole disc (110,000 ly) down to about 500 ly across by powers of two
  (`+`/`-`, the wheel), centred on the ship; the arrows and the mouse pan, `C` or `Space` bring the
  view back to the ship. Under 500 ly the neighbourhood page is the map.
* **What the explorer has on it.** The ship (a crosshair, `YOU`), home (the star map's amber
  diamond), the systems visited (dots, and the route between them in the order they were reached:
  a `route` line per arrival in the guide, so an older guide draws its visited stars without the
  lines; R-414, 2026-10-11: the route is gone, the user finding it confusing; the dots stay), the remote target with the ship's line to it (in a flight the ship moving along it), the
  explorer's named stars at the closer zooms, and the signals the radar locked (`Guide::signals`,
  rings in the radar's red: KI-349's "on no map or screen of the ship"). One panel line:
  `13,050 LY FROM THE CORE - SPIRAL ARM - IN THE PLANE`, the scale bar and the zoom.
* **The cost.** Measured 2026-10-08: the whole `galaxy` mode (its 400 x 400 face-on map at 61
  samples a pixel, the edge-on map, the band, the profiles) runs in 0.48 s on 8 threads. The chart
  builds its image at the logical resolution on the worker pool: the whole disc once a session, a
  zoom or a pan on a worker, coarse first, the last image shown meanwhile. Nothing of it touches
  the 3D frame.
* Done when: `vesperis_test chart` (new) renders the chart at every zoom from home, from the core
  and from 2,000 ly over the plane, printing each level's build time; a unit check puts the ship's
  marker within a pixel of its place at every zoom and the route through three arrivals in their
  order; the flow opens the galaxy page; `bench check` unchanged; the help's star map line says
  `TAB` (merged, not added: page 0 holds 21 lines).
* **Done 2026-10-08** (`game/galaxy_chart.cpp`; `10-decisions.md` G-05, KI-366). One change from the plan: the face-on column
  has a closed form (`galaxyFaceOnColumn`: the disc's exponential and the bulge's gaussian integrated exactly, the cap's slab
  bisected in the core; 0.01% from a 2 ly integration, 65-80 ns a column), so the images build in 2-7 ms on the pool when the
  view changes, with no worker and no coarse pass. Level 0 is the whole disc centred on the galactic centre, levels 1-8 follow
  the ship until panned; the light is toned to the core's column with the arms' young stars weighted brighter (home's own arm
  did not read on counts alone). Unit 208 (three new), flow 30 s, regress 22 unchanged, `bench check` within every budget.

### G-06: the next jump chosen on the chart

R-407 jumps along the aim by a number of light years: the explorer turns the ship toward a place it
cannot see and guesses the distance. On the chart the place is chosen and the jump plotted to it.

* **The cursor** (the arrows, Shift faster, the mouse: the R-408 sector map's cursor is the model)
  and its readout: the distance and the way from the ship (`galacticHeading`: COREWARD, RIMWARD,
  ALONG THE DISC, NORTH or SOUTH OUT OF THE DISC), the region (`galaxyRegion`'s names), the stars
  there (`galaxyDensity` at the point: `ONE STAR IN 9 SECTORS`, `NO STARS`), the flight's length
  (`vimanaSeconds`) and what the explorer has there (home, a visited system, a locked signal, a
  named star, a mark). The cursor's point lies in the plane by default, where the stars are;
  `PgUp`/`PgDn` move it over or under the plane on the edge-on strip.
* **Plot, then fly.** Enter plots: the star nearest the end of the line from the ship to the
  cursor's point (`jumpTarget`, one code path with R-407's jump: in the void the jump falls back
  along the line to the first stars and says how short it fell) becomes the remote target, drawn
  on both pages with its line, the readout `PLOTTED: A STAR 2 LY FROM THE CURSOR - 3,180 LY - 57 S`.
  Enter again flies (the Vimana, the log's `JUMP` line saying it was plotted on the chart); Esc
  keeps the target for `V`.
* **A look before the leap.** On a plotted jump `Tab` opens the neighbourhood page round the
  plotted star instead of round the ship (a `StarNeighborhood` at the end, 1.4 ms), where another
  star of that place can be taken as the target.
* **What the explorer has, as stops.** `N` steps the cursor through the locked signals, home, the
  named stars, the marks and the visited systems, so a signal locked 3,000 ly off is two keys from
  a jump.
* **The explorer's marks of the galaxy** (decided in): `L` names the place under the cursor (a void, a
  cluster, a stretch of arm), kept in the guide and lent like R-408's marks on a world, drawn on the
  chart with their names.
* Done when: unit checks plot from home to a point 5,000 ly away and arrive at a star within a few
  ly of it after `vimanaSeconds(5000)`, plot into the halo and fall short as R-407 does, keep a
  plotted target through Esc, and round-trip a galaxy mark through the guide and its export; the look
  before the leap lists the end's stars; the flow plots, looks, flies and finds the ship at its new
  place on the chart.
* **Done 2026-10-08** (`game/galaxy_chart.cpp`; `10-decisions.md` G-06, KI-366). The cursor carries the view (G-05's pan became
  the cursor pushing it at the face's edge; the zoom keeps the cursor's pixel); the readout uses the chart's rows (the region and the
  stars there over the face, the jump's line under the strip where the legend is while the cursor sits on the ship; the zoom moved
  into the scale bar's box), and the map pages now show the status. R-407's `startJump` is two steps, `plotJump` and `flyJump`, and
  the `JUMP` line is written as a plotted jump's flight starts (`- PLOTTED ON THE CHART`). The look leaves the plotted star out of
  the neighbourhood page's pick, or no other star there could be taken. Unit 213 (five new), flow 31 s, regress 22 unchanged,
  `bench check` within every budget.

### G-07: the crossing

A jump across the galaxy (87 s since R-407) shows the neighbourhood's streaks and no galaxy: the
band is not drawn in a flight (KI-364). Drawn round the ship as it goes, a jump coreward would show
the bulge swelling and the band opening round it, a jump north the disc falling away into a whirl.

* The band rebuilt from the ship's place along the flight (`buildGalaxyBand`: 7.8 ms on 8 threads,
  measured 2026-10-08) every half second on a worker and crossfaded, the nebula patches likewise,
  the streaks as now; the chart's ship moves along its line (G-05).
* Done when: frames along a 10,000 ly coreward jump and a 3,000 ly northward one, `bench check`
  gains a flight frame within its budget, `stability` on a flight shows the band steady.
* **Done 2026-10-08** (`10-decisions.md` G-07, KI-367). The band is built ahead of the ship, not behind it: the flight's line
  is closed-form, so keyframes at flight times (half a second apart, closer where the core turns more than two degrees between
  two) are built a share a frame on the pool inside the frame (not a worker thread: the pool is not reentrant, and a thread's
  timing would make the frames depend on the machine) and blended; the first is the band the ship was seeing, the last is handed
  to the parked band, so neither end builds at once. The nebula patches are not crossfaded but faded by the flight's speed (at a
  crossing's pace a complex passed in a frame or two as a 30-degree disc). A flight straight out of the disc turns its right side
  to the galactic centre (the cabin's floor has no window). `vesperis_test crossing` (frames through the cabin and bare),
  `stability flight` (the band steady: no pixel over 2 levels a frame), `bench check`'s `vimana 2x`; unit 214 (one new), flow,
  regress 22 unchanged.

## Phase 2: groundwork

Source: `IDEAS-civilisation-expansion.md` (the request in full, what exists, and a section per milestone
with its lists), with `docs/reference/12-civilisations.md` for the C series the peoples rebuild and the C
sections of `10-decisions.md`. Read the milestone's section of the ideas note before starting it.

### H-01: research, grammars and histories

The user asked for the investigation first: "investigation on free-context grammar is required ...
This context-free grammar stuff would also need investigation and research on your side". A
context-free grammar gives variety without memory (agreement, one pronoun for one person, an event not
told twice all need more), and the depth of Dwarf Fortress's and Caves of Qud's texts comes from the
history they tell. The research reads the sources of the ideas note's section 11, answers its seven
questions (a plain weighted grammar, an attribute grammar or a grammar over a realiser; the history
first; long texts by genre schemas and salience; the authoring format; cost; evaluation; the language
layer) and tries the candidates in a scratch build on C-02's world inputs. It also fixes the shape of the
data the peoples' milestones lay down for the grammar to read (the profile's fields, the history's
events, the inputs list), so A-01 and H-02 are built to it.

* Done when: `docs/ideas/RESEARCH-records.md` holds the survey (each source: what it does, what it
  would give Vesperis, what it costs), the formalism and the authoring format chosen with the reasons,
  the evaluation plan, the inputs list and the data model; the prototype prints fifty texts of three
  genres for two test worlds for the user to read beside C-02's; the decision is in `10-decisions.md`;
  the prototype is out of the tree (CLAUDE.md's rule for experiments).
* **Done 2026-10-08** (`RESEARCH-records.md`, `RESEARCH-records-samples.md`, `10-decisions.md` H-01, KI-368). The history
  first, told by a small attribute grammar over a realiser: weighted alternatives with guards, bound entities, attributes
  down (the tense, the facts) and up (a part's choice), dryness in a record and across a world's catalogue, the record's plan
  as directives (salience picks the events, a genre's schema orders them), and a realiser that owns the English (references
  and kinship, pronouns where clear, verbs, numbers with their nouns). The format is plain `.vg` files embedded as C++ raw
  strings (no build step) and reloaded by the harness. A scratch prototype on C-02's inputs (a history stub, 579
  alternatives) printed twenty-five letters, chronicles and epitaphs on each test world (the Rauzhia set medieval, the Feloya
  industrial from year 309) and one plague from three sides. Composing the frequent parts cut the repeated sentence frames
  over 100 chronicles from 77% to 30%; the prototype's records repeat less than C-02's at 100 and 200 a world (21-22% and
  33-35% against 34-38% and 50-51%) at half as long again, 28 µs a record. The data model A-01, H-02 and T-04 build to is
  the note's section 6. Nothing in the game changed.

### A-01: the ages

Every people gets a profile (ideas note section 1, in the data model H-01 fixed): the age it reached
(prehistoric, medieval, industrial, modern, futuristic, far future; the early ages commoner, the far
future rare), the ages it passed through, the cause of its collapse (by the age and the world), how long
the collapse took, how long ago it ended (C-08's span, longer for the late ages), how many they were and
how much of the world they held. One function, read by every later milestone. Only the radio ages
transmit: C-07's rule gains the age (C-15, next, pins the salt and the powers on the transmitters left).
Generation 13.

* Done when: `vesperis_test peoples ages` lists the first hundred peoples near home with their age,
  cause, end and extent, the shares as decided, and the nearest people of each age to home (the new
  test worlds, one per age, printed and pinned); unit checks (the profile deterministic, every
  transmitter of a radio age); the regress re-blessed where it must be, `GEN_VERSION` 13 recorded in
  `10-decisions.md`.
* **Done 2026-10-08** (`galaxy/peoples.*`, `10-decisions.md` A-01 and the GEN 13 row, KI-369, KI-370):
  * the six ages by the decided shares (within 25 ly of home 28.2/33.0/17.0/12.5/7.1/2.1% of 1,501 peoples);
  * the eras in the people's own count with their governments;
  * radio for 29%;
  * the end by the age's span, with the ruins weathered over the age's durability;
  * one of 37 causes by the age and the world, with the collapse's length and stages (two peoples that ended together share
    one);
  * the extent, government, economy, faith and values, all named.

  Only the radio ages transmit: the candidates went up 3.4 times and the salt was re-pinned. Home hears a modern, a
  futuristic and an industrial people at 90, 184 and 197 ly. `vesperis_test peoples ages` prints the hundred, the shares and
  the nearest of each age; the six test worlds are pinned in `AGE_WORLDS`. Unit 217 (three new); the regress unchanged.

## Phase 3: the faint

Sources: `docs/iteration-3/plans/requests/R-410`, `IDEAS-ruins-shards-signals.md` section 2 and the C-07
section of `10-decisions.md` (the radar's register: found, not handed; the system named, never the
source; the sound tells), KI-349, and the ideas note's section 10 for each age on the air.

### C-15: deep listening, the radar's gain

The radar hears the transmitters within 200 ly (`SIGNAL_REACH_LY`) and the loud pulsars within 25,
at one sensitivity; what it hears is as old as it is far. Turned up, it should hear farther, at
the price of the noise.

* **Why not a longer reach.** Measured 2026-10-08 with a scratch program over `signalsNear` from
  home: 3 peoples' signals within 200 ly (1 ms), 9 within 400, 59 within 800 (7 ms), 234 within
  1,600 (30 ms). The scan stays cheap, but a wider reach fills the sky: "rare and far" (the ideas
  note, C-07's decision) would be gone and the meter would read a forest of lobes.
* **A power per transmitter.** Each transmitter sector gets a power from a hash of its own (most
  weak, a long tail of strong ones), and the gain carries a transmitter beyond the 200 ly as far as
  its power allows (the received strength falling with the square of the distance), so each step
  up adds a few far, faint signals and the top step a handful from thousands of light years. At the
  first step the radar hears what C-07 hears under A-01's rule (the radio ages' peoples, the strengths
  as before). The loud pulsars likewise. The salt and the powers are pinned together after a scan, as
  C-07's salt was: home hears at least two peoples at the first step, and every step adds one to three
  from home and from the test worlds; the strongest are heard to about 6,000 ly.
* **The dial.** In the radar camera `+`/`-`, the wheel and `PgUp`/`PgDn` (the telescope's keys: the
  two are never out together) step the gain, x1 to x32 in six steps, written by the scope
  (`GAIN X8`). The reach is not printed: it is the transmitter's, not the radar's.
* **The price.** The static's floor rises with the gain (the meter's base climbs and wanders, the
  scope fills with grass), so a far signal is a small rise over a high floor; the hold to lock grows
  with the gain and the faintness (2.5-5.5 s today, ten and more at the top), the lock keeps within
  a narrower cone, and a strong near signal saturates (the meter pinned, the sound overdriven):
  backing the gain off is part of the play.
* **The far scan** runs on a worker when the gain steps up or the ship's sector changes
  (`LISTENING DEEPER...` meanwhile); the first step keeps today's 1 ms scan on the main thread.
* **The readout stays C-07's:** the system only, its distance and its age (`THE SIGNAL IS 3,180
  YEARS OLD`); Enter flies. A locked far signal is on the chart (G-05), two keys from a jump (G-06).
* Done when: `vesperis_test signals` prints, per gain step, the peoples' signals and the pulsars
  heard from home and the test worlds (count, nearest, farthest) and the scan's cost; unit checks:
  x1 hears C-07's list under A-01's rule, a far transmitter is heard from its step and not below, the
  hold grows with the gain; the flow turns the gain up, locks a far signal and flies to it.
* **Done 2026-10-08** (`galaxy/signals.*`, `game/radar.cpp`, `10-decisions.md` C-15, KI-370 rewritten, KI-371):
  * the dial x1, x5, x11, x18, x25, x32: even steps, since a doubling dial adds 0.6 to 4.4 peoples a step from home and
    these about two each (the reach's area grows with the gain);
  * a power per transmitter: C-07's sectors weak, and a sparse population of strong ones in cells of 256 sectors heard to
    about 7,000 ly at x32; every transmitter within 200 ly at every gain, so x1 is A-01's exactly;
  * beyond 200 ly a people is heard only in the light of its first broadcast (`airYears`), so the far signals are nearly all
    machines;
  * the salts pinned together: every step adds one to three peoples from home and from all nine test worlds. From home:
    3, +3, +1, +1, +2, +2, to 5,524 ly;
  * the floor, the overload (the meter pinned red, the hold refused, the sound overdriven), the hold x2.25 and the cones
    narrower at x32, and a lock lost when the gain goes under it;
  * the far scan in slices of 40,000 cells a frame (`LISTENING DEEPER...`) instead of a worker.

  Unit 219 (two new), flow with the far lock at 5,523 ly, regress unchanged. No generation change.

### C-16: what the gain lets in, and each age on the air

The user's words are the brief: "it should give some radio noise, creepy mixed sounds when you are
trying to get really far signal". C-07's receiver (the bed, the signals' own voices, the
programmes, the static's events, the echo) is the instrument; the gain is how much of the void
comes through it. The register stays C-07's: the sound tells, the readout never does, and nothing
is explained.

* **The floor.** The bed rises with the gain: the hiss, a crackle of clicks, the rumble breathing
  harder.
* **The galaxy's own noise.** Turned toward the band and the core the receiver hears more (the
  band's brightness in the beam's direction, from the map the sky already has: `sampleGalaxyBand`),
  toward the poles less: the sky has a radio face, and a far signal toward the core is the hardest
  to pull out.
* **Mixed.** The signals in the wide lobe bleed in under the one centred: two broadcasts at once,
  one through the other, a pulsar's thump under a voice; heterodyne whistles between their carriers
  that slide as the beam moves; at the top steps a metallic ring of intermodulation.
* **The far voices worn.** A far signal's content comes through a narrower band, with deep slow
  fades, two or three echoes of itself, its pitch drifting and dropouts, so a voice 3,000 years old
  is the shape of a voice under the noise.
* **Voices in the static.** C-07's events (a carrier, a boom, a moan, knocks, a murmur, a breath, a
  scream) come more often with the gain, and new ones join at the top: a word of a recording the
  explorer has heard coming back out of the static (from `Guide::heard`, never one not heard),
  speech running backwards, counting whispered in no one's language, a breath close to the
  microphone, a knock answering a knock, a choir swelling and stopping. None is logged.
* **False rises.** At the top steps the meter rises where nothing is; held, the hold fills and
  empties (`NOTHING THERE`). Noise makes ghosts; the radar stays honest as an instrument.
* **Each age on the air** (ideas note section 10, here rather than at H-05 so the receiver is designed
  once): the late industrial age's spark-gap Morse and a phonograph's broadcast through its horn; the
  modern age's voices, music, numbers and a carrier's buzz (C-07's programmes are the modern age's); the
  futuristic age's data, synthetic voices and beacons; the far future's beacon that is mathematics. What
  a programme carries is the people's own catalogue, so the records of H-04 and H-05 reach the air
  without coming back here.
* **The picture.** The camera's static and snow grow with the gain, the band glows through the
  phosphor (the radio sky the receiver hears, KI-349's "not a radio sky"), the star field ghosts a
  faint double, the rolling band runs faster, and the picture tears now and then.
* **The levels from renders.** `vesperis_test signals wav` gains renders of the bed, a near voice
  and a far voice at three gains and each age's programmes, with their rms and peaks printed; C-07's
  lesson stands (its first bed clipped at an rms of 0.25): no peak over 1 and the rms within C-07's at
  every gain.
* Done when: the renders hold their levels at every gain; a unit check of the top step's variety
  (more events a minute, two sources in the wide lobe both on the air, a false rise that never
  locks) and of the programmes by age (an industrial people's broadcast is Morse or a phonograph, never
  a data burst); the report says what to listen for and where (the signal, the gain, the direction),
  and the user's ear judges it, as it did C-07's second call.
* **Done 2026-10-09** (`game/audio.*`, `game/radar.cpp`, `galaxy/signals.*`, `10-decisions.md` C-16, KI-349 and KI-370
  narrowed, KI-371 closed, KI-372):
  * every signal's sound in a channel of its own: the loudest other within forty degrees heard under the one centred above
    x1, a whistle between two carriers sliding as the beam turns, a metallic ring at the top steps;
  * the floor rising with the level held at C-07's; the galaxy's noise from the band's map (the core's direction the hardest);
    far voices worn by their distance;
  * the static's events oftener and, from x25, five stranger kinds; a word heard before coming back; false rises that a hold
    empties (`NOTHING THERE`);
  * each age on the air: a phonograph and Morse by spark (the text in the people's own code), C-07's programmes for the
    modern age, a synthetic voice, data and beacons, a beacon of mathematics; each age's carrier;
  * the picture: the static, the band's glow, a double, the rolling band, tears.

  Unit 222 (three new), every render within C-07's levels, flow as C-15's but the radar's lines, regress unchanged. No
  generation change.

## Phase 4: what they built

### A-02: the forms the renderer needs

The ruins are boxes and domes (`RuinElem`), three levels of detail, drawn within about 5 km, in the
ground's rock or sand bank. The ages need the forms of the ideas note's section 4 (cylinders and cones,
vaults and arches, domes of several profiles, pyramids and steps, spires, shells, trusses, cables,
rings, extrusions and turned profiles, grown forms) with their broken states, windows as a pattern on a
face, colliders with their heights, levels of detail down to a silhouette and a skyline band at the
horizon (a city of towers seen from thirty kilometres), and a people's palette of three or four ramps in
the surface's free banks (18-20 and 24-31).

* Done when: a harness sheet draws every form whole and broken at 1x-4x near, middle and far; a
  skyline frame from 30 km; `bench check` gains a city frame (2x within 16 ms); `stability` on a
  skyline shows no shimmer; a unit check walks into each form's collider; the serial baseline
  (`VESPERIS_THREADS=1`) is bit-identical.
* **Done 2026-10-09** (`galaxy/forms.*`, `surface/forms_draw.cpp`, `10-decisions.md` A-02, KI-373):
  * twelve kinds and their broken states; C-01's settlements draw to the pixel as they did (`RuinElem` became `Form`);
  * a level of detail a building, a silhouette beyond 4 km (the skyline), big forms on the highest ground under them with
    their foot down to the lowest, on the ground the rings draw;
  * colliders for every form; a people's four colours in banks 18-20 and 24 (25-28); windows as a pattern on a face;
  * built on the workers, drawn in bands: the serial loop's frame.

  `forms sheet` at 1x-4x, `forms skyline` from 30 km, the bench's city 2x at 8.3 ms, `stability skyline` steady, unit 226
  (four new: the forms, walking into each, the banks, the serial frame), regress unchanged, flow as C-16's. Not as the plan
  had it: the skyline from 30 km is a ghost on the peoples' hazy worlds (the forms take the terrain's fog; an airless world
  shows the city clearly), and only the cells within two of the explorer's are gathered (a city farther off is the harness's
  until a world's plan says where cities are). No generation change.

### A-03: what each age builds

The buildings of six ages (ideas note section 2: dwellings, the civic and the sacred, the works, the
density and the heights) replace C-01's ten kinds, built from A-02's forms; the decay is the material's
(wood gone in centuries, dry stone standing millennia, iron to rust, concrete to its bars, glass gone at
once, the far future's untouched), by the age and the rain as C-08's was. Every age's buildings exist as
generators, so a people's older works can be built in their own age's way (where they stand, an old core
inside a city or megaliths in the fields, is T-02's, from H-02's eras). Interiors where the records will
lie (T-04). Until T-02 the buildings stand in C-01's settlement plans.

* Done when: the `ruins` mode renders a settlement of each age at three ages since the end (a wooden
  village after a thousand years is rings of post holes, a concrete tower after three thousand a
  stump, a far-future spire after a hundred thousand still sharp); a scene per age; unit checks of the
  decay by material.
* **Done 2026-10-09** (`galaxy/buildings.*`, the plans in `galaxy/ruins.*`, `10-decisions.md` A-03, KI-374; KI-373 and KI-350
  narrowed):
  * about forty-five kinds of building, each a generator of A-02's forms in its age's way (C-01's ten the medieval age's, roofed);
  * C-01's plans hold them (until T-02), spaced by the age, and the share of cells holding a settlement follows the age (the far
    future's one structure in a cell of about seven hundred: one to a few a world waits for T-01);
  * the decay is the material's: ten materials, masonry C-08's to the digit, the real years since the end; nothing rests on what
    fell; a tower's top lies where it fell; glass gone at once;
  * every building with a room says its floor, its doorway and the spots where a record can lie: the shards lie there;
  * a young ruin is a clearing the forest takes back.

  `ruins ages` (each age at 100, 1,000, 10,000 and 100,000 years: a wattle village after a thousand years is rings of post holes, a
  concrete tower after three thousand a stump, a far-future spike after a hundred thousand as sharp as in its first year), `ruins
  kinds` (every kind alone), a scene per age (`civilisation_<age>`, on peoples that ended lately), unit 230 (four new: the decay by
  material, what each age builds, its pieces resting and its rooms open, the decay's examples; C-01's, C-03's and C-08's checks
  read every building's room), regress unchanged, flow as A-02's but the landing zoom's roads (the industrial world's new towns),
  bench check within every budget, the scenes and the sheet the serial loop's frames. No generation change.

### A-04: the architecture genome

The randomiser (ideas note section 3): thirty-five deep variables per people kept through its ages
(mass and proportion, form, surface, colour and light, place, meaning: the focal accent, "what to look
at") and the age's own (materials, techniques, heights, spans), so no two peoples look alike and a
people looks like itself in every age. Every variable acts at a scale the picture shows (silhouette,
proportion, rhythm, colour), the colours in the people's banks.

* Done when: the architecture sheet (a dozen peoples per age at the same light, their genomes printed)
  is rendered and shown to the user; a distinctness measure finds no two of a hundred peoples near home
  alike on the strongest variables; a people's buildings of two ages side by side read as one people's
  (the user judges); a unit check that the genome is deterministic and every variable reaches the
  drawing.
* **Done 2026-10-09** (`galaxy/genome.*`, the genome's ways and palette in `galaxy/buildings.*`, the plans in `galaxy/ruins.*`,
  `10-decisions.md` A-04, KI-375; KI-374 narrowed):
  * thirty-five deep variables a people draws from its seed and keeps through its ages (a hash each), read through its age
    (`cultureWays`): the plan's forms, nine roofs in every age's material, five profiles, the windows' shape, rhythm and share,
    ornament, motif, an expressed frame, tops, colours (a base, an accent where the people puts it, a finish, a glass tint, the far
    future's light), the relation to the land, the facing, the streets, public space, grouping, the edge, links, the focal accent,
    the centre, the monuments, the dead, the houses' sameness, built light or to last; the age's technique drawn by their leanings;
  * the genome's works (a focal accent of ten kinds in every age's way, a windmill, an aqueduct, joined towers, a tunnel's mouth) and
    the kinds' new styles (market halls, a palace, a round temple, an amphitheatre, a mall, monuments);
  * C-01's ring segments (rotundas; A-03's huts, cisterns, observatories, stone circles) laid along the ring, not across it.

  `genome sheet` (the twelve peoples nearest home on one ground under one light, a page an age, their genomes printed), `genome ages`
  (a people a row in every age), `genome distinct` (the hundred nearest: the nearest pair 4.93 of 13.4 apart, alike under 3.4),
  `genome pair`, `genome reach`, `genome print`, `ruins kinds`' page of the genome's works; unit 233 (three new: the genome
  deterministic, every variable reaching the drawing in every age it acts in, no two of a hundred alike), regress unchanged, flow as
  A-03's but the zoom's roads, bench check within every budget. No generation change.

## Phase 5: where and when

### T-01: a people's map of its world

Settlements stop being hashed cell by cell (ideas note section 5): a coarse reading of the world
(land, height, slope, water, coasts, rivers, climate, the dead seas, the divide of two peoples) scores
where each age would settle, and a plan lays down a capital, cities, towns, villages and hamlets,
spaced as the age travelled, with their networks (paths, roads, rail and canals, highways, maglev) over
as much of the world as the people held, each place with its rank in the founding (for H-02). The 2 km
cells read the plan; C-09's roads become its edges; the landing map and its zoom show the footprints and
the globe the greatest works (what is seen from orbit is what is walked on); the scanner's RUINS mode
hears the plan's places by size. Built once a world on a thread at the approach.

* Done when: `vesperis_test plan <sx> <sz> <body>` draws a world's plan over its map for each age and
  prints the build time (about a second on a thread); unit checks (no settlement in the sea, the capital
  on good ground, the networks connected, a city spanning cells drawn whole); the landing map of a
  modern people's world shows its cities; `landmaps` and the descent's first frame within budget.
* **Done 2026-10-09** (`galaxy/plan.*`, the cells' reading in `galaxy/ruins.*`, the ways on the ground in `galaxy/roads.*` and
  `surface/site.cpp`, `10-decisions.md` T-01, KI-376; KI-352, KI-373, KI-374 and KI-375 narrowed):
  * a coarse reading of each civilised world (165,018 cells of half a degree: the land of the people's time, the height, the roughness,
    the climate, the fertility, the coast, coal and ore, the rivers of a flow over the grid), scored for the age and the people's
    relation to the land; the territory grown from the capital to the profile's held share; the capital, the cities and the towns by
    rank (a prehistoric people's gathering place and its seats, a futuristic people's great places, the far future's one to three
    works), spaced as the age travelled, each with its founding rank, its population and the core the ground draws;
  * the networks: a greedy spanner over the places, the water dear, the trunk the routes between the capital and the cities (paths;
    roads; rail and canals; highways; maglev on its pylons; crossings over water), every place reached; the ways on the ground in legs
    between waypoints every 3 km, walked by C-09's walk with each kind's weights and drawn by their kind;
  * the cells read the plan: a place's cell holds it (a city's core spans cells and is drawn whole, the plan's places drawn to 24 km for
    the skyline), the countryside by the plan's density, nothing outside the land a people held; the landing zoom and the telescope's
    plate draw the cores and the ways; the scanner hears the places by their size; built once a world on a thread at the approach;
  * found on the way: C-09's road blocks stepped one longitude index in every row (cells away from a hamlet's neighbour far from the
    meridian).

  `plan <sx> <sz> <body>` (each age's plan over the world's map, the build time: 0.24-0.67 s on one thread for the six age worlds),
  `plan ages`, `plan view` (a capital through the telescope); unit 238 (five new: the plan deterministic and in time; its places dry,
  the capital on its territory's best ground, the networks connected; the cells reading it; a city's core drawn whole from another
  cell; the zoom, the scanner and the ways on the ground); the bench's `descent people` 57.4 ms of 80 and `capital 2x` 8.9 ms of 16;
  `landmaps` 4.6 s as before; regress unchanged; flow as A-04's but the zoom's step (round a town of the plan). No generation change.

### H-02: the history

A history per people, simulated before the towns, the works and the records that read it (ideas note
section 8): eras through its ages, the founding and growth over T-01's places (the capital first, by the
plan's ranks, some places abandoned early), figures, institutions that change, events (wars, plagues,
famines, migrations, discoveries; disasters read from the world: an impact, an eruption, the drying, a
flare, a long winter), and the collapse by its cause, in stages, to its end and its last place.
Deterministic from the profile, the plan and the world; a few hundred events; milliseconds.

* Done when: `vesperis_test history <sx> <sz> <body>` prints a people's history as Dwarf Fortress's
  legends would, one people per age for the user to read; unit checks of coherence (no event after the
  end, a figure alive at its events, every place on the plan, the collapse's stages in order); the cost
  printed.
* **Done 2026-10-09** (`galaxy/history.*`; `planScores` in `galaxy/plan.*`, each age's whole length and the world's read in
  `galaxy/peoples.*`; `10-decisions.md` H-02, KI-377; KI-376 narrowed):
  * a year at a time from the capital's founding to the end, deterministic from the profile, the plan and the world: the eras with
    the people's names for them, the government changed once in each (a kingdom founded, a revolution, the priests' rise, the machine
    given the rule), a faith preached where the era's is new; a few legends of the old time before year 1;
  * the plan's places founded in its order, each in the era whose ground suited it (a modern people's old cities medieval, its mill
    towns industrial), a few left early, every one left by the end in the cause's order, the least exposed the last place; a later
    people's cleared cities kept as places;
  * the rule by its form (a monarchy's houses, heirs and regents, chiefs and elders for life, consuls and speakers by their terms, the
    machine's rule), the figures, ten families followed through the last 600 years with their births, marriages, moves and deaths;
  * the events: wars with their battles, sieges and peace, sicknesses in waves, harvests and famines, the world's own disasters
    (floods, droughts, earthquakes, eruptions, stones from the sky, flares, long winters), works by what they valued, the plan's ways
    opened, inventions at their share of each age, discoveries, faiths, institutions, deeds, unrest, migrations, omens (the system's
    comet by its period); two peoples' meetings, treaties, wars and a conquest held the same in both their histories;
  * the collapse by its cause in its four stages, its signs at the places most exposed, its harm, the places left with their reasons,
    the rule's end, the last person at the last place; every event's cause kept (a famine its harvest, a battle its war, a death its
    sickness or battle in its season, an accession the death that opened the rule).

  `history <sx> <sz> <body> [second]` (the legends: the head and the build time, the eras, the rulers by house, the places, the wars,
  the sicknesses, the inventions and institutions, the figures, the families, one family's years, the chronicle, the collapse to the
  last person), `history ages` (one people per age), `history check`; 0.9-6.5 ms a history on the six age worlds (1,100-4,800 persons,
  2,900-10,200 events, 330-3,435 of them in the chronicle); unit 243 (five new: the same twice and quick; every event in the years,
  its persons alive and its places the plan's and standing; the collapse's stages in order to the last place; causes before effects,
  battles within their wars, the kin and the reigns coherent; two peoples' shared events the same in both histories); regress
  unchanged; flow and bench as T-01's. No generation change.

## Phase 6: towns and works

### T-02: towns by age

Towns as each age built them (ideas note section 6): camps and rings, walled towns round a castle or a
temple with their fields, industrial grids round the mills and the station, modern grids with a centre
of towers, a ring road and suburbs, arcologies and domes, the far future's absence of towns; districts,
height and density falling from the centre, an edge; the old cores and layers by H-02's eras (a
medieval core inside a modern city, megaliths in its fields, a town abandoned early in its older style);
C-08's reading of the site by age (a quay, a dock, a container port; terraces, fields, irrigation
circles; cisterns, reservoirs); the street plans from the genome's street pattern.

* Done when: a scene of a town of each age (`civilisation_<age>`); a modern people's world shows its
  older layers; the districts and C-08's features by age drawn; the bench's city frame within budget;
  unit checks (no building across a street, the colliders walkable, an old core's age older than its
  city's).
* **Done 2026-10-09** (`galaxy/towns.*`; `layoutTown` and the towns' pasts in `galaxy/ruins.*`; the harbour's and the water's works in
  `galaxy/buildings.*`; the ground's marks in `surface/site.*`; `historyOfWorld` in `galaxy/history.*`; `10-decisions.md` T-02, KI-378;
  KI-377 narrowed):
  * the streets first, in the people's pattern and each age's way (a medieval town's crooked and narrow, an industrial quarter's straight
    with back lanes, a modern city's blocks of a hundred metres, its ring road and its suburbs' cul-de-sacs; a village a street or two, a
    green, lanes), the main ways out along the plan's ways, the blocks widened to what the class builds;
  * the buildings on the streets' lots by district: the centre's square (its centre in the oldest layer's way), the old core, the houses
    (densest at the centre and on the main streets, rows along a medieval main street), the works downwind, the harbour, a modern city's
    towers round its old core and suburbs thinning into its fields, the station, the market hall, the stadium and the power station; the
    height and the density falling from the centre; the edge a medieval wall with its gates where the main streets go through, a ring road,
    nothing;
  * the layers by H-02's eras: a city in rings of its ages, each in its age's way (a modern capital's medieval core, industrial ring and
    modern rim), the old wall's line a boulevard, a place left early in its older way and weathered from then, the old stones in a capital's
    fields; the countryside's villages and towns older by the history's shares, its lone monuments megaliths;
  * C-08's reading of the site by age: a quay, a dock with its jib cranes, a container port with its gantries and containers; terraces;
    cisterns and reservoirs; the fields (plots, strips, walled fields, enclosures, irrigation circles, terraced fields); the streets,
    squares and fields drawn into the ground.

  `towns <sx> <sz> <body> [second]` (the plans from above), `towns ages`, `towns view` (the capitals through the renderer from the air, over
  the old core and along a street; a harbour, a reservoir and the fields of each age), `towns check`; `scene civilisation_<age>`; unit 248
  (five new: no building across a street over 41 towns; the streets walkable and joined; an old core older than its city, a place left
  early of its older age; the districts by age; a harbour of each age and the ground's marks); regress unchanged; flow 33.6 s, line for line
  H-02's; bench within every budget (`capital 2x` 8.7 ms of 16, `city 2x` 6.2, `descent people` 71.1 of 80). A capital laid out in 10.6
  ms. No generation change.

### T-03: the works

"It can be virtually anything" (ideas note section 6): quarries and mines, dams and reservoirs, canals
and aqueducts, bridges, observatories, shrines and pilgrims' roads, necropolises (C-12's graves
widened), geoglyphs, megaliths, fortification lines, battlefields, launch sites, antenna fields, wrecks,
colossi, terraformed land, the last shelter, the far future's inexplicable; each placed by its rule (a
dam at a valley's narrows, a quarry in the rock the town is built of, an observatory on a summit, a
launch site near the equator) or by the history (a battlefield where H-02's war was fought, the last
shelter of the collapse, a dam of its era), heard by the scanner, marked by the explorer.

* Done when: a harness gallery shows each work of each age where its rule put it on a test world; unit
  checks of the rules (a dam across a valley holding a reservoir, a quarry in its rock, a battlefield
  between the two sides of its war); the scanner's modes hear them.

**Done (2026-10-09):** `galaxy/works.*`, the ground's edits in `surface/site.*` (`docs/reference/12-civilisations.md`, "The works"):
  * twenty-one kinds, each where its rule or its history puts it (the table there), built in the way of the age that made it and weathered
    from when it was left: the history's battles (each war's greatest, between its two sides), its last place (the last shelter) and the
    works it finished (dams, mines, aqueducts, observatories, lighthouses, launch sites) first, then the land's (quarries, mines, the shrine
    and its way, necropolises, the plains' lines, henges, the fortified line, antenna fields, wrecks, a colossus, terraformed basins, the far
    future's, wind farms);
  * a dam fitted to its valley's narrows by its river town or up its river, its reservoir held behind it as high as it holds (breached and
    drained when its wall is half gone; an old bed on a dead desert), never in a basin the drainage fills (a priority flood on its grid);
  * their pieces as a settlement's (twenty-two new generators; heights over the reference level for a dam, an aqueduct, the lattice), their
    ground in the terrain (cuts, pits, ditches, craters, plains made flat, the reservoir's water, lines, terraces);
  * heard by the scanner's WORKS mode after RUINS; marked by the explorer.

  `works <sx> <sz> <body>`, `works ages`, `works gallery` (the gallery: each kind of each age's scene world through the renderer, a sheet per
  age), `works map` (each work on the ground with the place it serves), `works view`, `works check`; unit 254 (six new: a dam across a valley
  holding a reservoir; a quarry in its town's rock; a battlefield between the two sides of its war; the scanner hearing them; the works the
  same twice, out of the cores, every kind laid, their forms sound; the WORKS mode in the game); regress unchanged; flow 33.3 s, line for
  line T-02's; bench within every budget (`descent people` 70.9 ms of 80, `capital 2x` 8.7 of 16). No generation change; KI-379.

### T-04: where the records lie

The shards' places and objects by age (ideas note section 7: caves, scriptoria, archives, broadcast
stations, data vaults, the archive that keeps itself; carved bone to crystal), archives holding several,
the last recording somewhere that means it (the history's last place), how many records a people left
(80 to 200 by its age and size: fixed here, before the texts are written); C-03's rule kept (a copy
taken is gone everywhere). The shards found before generation 13 are kept by their index (the counts and
the language kept, the contents new), with one status line on the first load.

* Done when: the harness prints a world's record sites by age and place; each object drawn and taken
  (`E`) in a scene per age; unit checks (no two copies in one settlement, the last recording at its
  place, an old guide's found shards kept by index after the change).

**Done (2026-10-09):** `galaxy/records.*`, the objects in `surface/surface_view.*` (`docs/reference/12-civilisations.md`, "Where the records
lie"):
  * 80 to 200 records a people by its age and its numbers at its height (the age worlds 86, 101, 135, 143, 175, 178), C-02's fifty the first
    (their texts, music, charts and last recording kept), the rest at the fifty's shares of music and charts;
  * each dated in the people's own count from the founding to the end, the later years the likelier (what each age wrote, what the years
    left), C-02's order kept and the last recording the end's; the shards' years, the decoder's dates and the graves' read the history's
    count now;
  * made in its year's way and drawn as that object: carved bone, carved stone, a painted wall; clay tablets, scrolls, codices, an engraved
    plate; printed pages, wax cylinders, glass plates; reels, discs, drives; crystals; something that hums;
  * each with a home among its people's places standing in its year, which always holds it (every record lies somewhere on the world),
    copies elsewhere; a settlement's few in the buildings its age kept writings in; an archive's row across the room in the capital and every
    city and a third of the towns (a prehistoric chief's longhouse; the far future's rings round its structures); a few in the works that kept
    records (the last shelter, observatories, necropolises, shrines, stones, an antenna field's mast, a launch site's blockhouse, lighthouses);
    the last recording at the history's last place in the building its age keeps last words in and in the last shelter by it; nothing written
    after a place was left;
  * the shards found before kept by their index (the second people's moved from 50 + index to 1000 + index, with their decoded, heard, lent
    and named lines), said once on the first load.

  `records <sx> <sz> <body> [second]`, `records ages`, `records scenes [age]` (each age's archive and objects drawn and taken with `E`, a sheet
  per age), `records check`; unit 259 (five new: the catalogue; no two copies in a place, none after its place was left, archives in every
  capital and city; the last recording at its place and its shelter; an old guide's shards kept by index; the objects taken in the game);
  regress unchanged; flow 35.1 s (the last recording's year now the history's, the decoder's word counts a word apart); bench within every
  budget (`descent people` 71.3 ms of 80, `capital 2x` 8.8 of 16). No generation change; KI-380.

## Phase 7: the records

### H-03: the grammar engine

The formalism H-01 chose, built: rules with weights, conditions on the inputs and bound variables
(one author, one subject, one event per record), the realiser (articles, plurals, tenses, pronouns,
agreement, punctuation) so that C-02's "him mother" and "the the well" cannot be written, the
authoring format with a harness that reloads it, and automatic checks. C-02's thirty-one kinds are
rebuilt on it first, from the same inputs, to prove it before new content.

* Done when: `vesperis_test grammar` prints thousands of texts with every automatic check at zero
  failures (doubled words, agreement, length, anachronisms); C-02's kinds produce their texts on the
  engine; the cost per text printed; the format documented in `12-civilisations.md`.

**Done (2026-10-09):** `galaxy/grammar.*` and `src/galaxy/grammar/*.vg` (`docs/reference/12-civilisations.md`, "The grammar engine",
the format; `10-decisions.md`, H-03):

* the format: rules and alternatives with weights, guards (facts, attributes, entities and their fields; orders by number or age; OR),
  `once`, tenses, a list's number, `keep`, `steady`, lengths; marks for rules, facts, choices, entities and their cases, paths, verbs by
  subject, numbers with nouns, attributes in reading order, kept words, persons a text brings in, plurals, directives; `@root`,
  `@lexicon`; compiled in as raw strings, read from disk by the harness;
* the link (every unknown reference and unreachable rule, "a" before a plural list: an error with its file and line), the expansion (the
  record's unused alternatives first, the people's catalogue memory), the realiser (pronouns only where clear, also of persons whose
  gender the reader cannot know from their names; verbs, numbers, plurals, articles, the seams), the checks and the measures;
* C-02's thirty-five kind rules on it, from the same inputs: the kin, the child (its own pronoun) and the founder as persons, C-02's
  draws in their order (the tones, pieces and charts as before), a people's texts one catalogue with C-02's kind weights and no text
  repeated whole, the charts' captions too.

  `grammar` prints 3,479 texts and 444 captions of 48 peoples (every text in `shots/tests/grammar_texts.txt`) with every check at zero:
  no mark left, no diagnostic, no doubled word, no wrong article, none before a plural, no pronoun against its verb, every capital,
  the spacing, every length, no anachronism; 3.9 us a text, 0.5 ms a people. Unit 263 (four new); regress unchanged; flow 34.0 s (the
  new texts' word counts and speeches); bench within every budget (`descent people` 72.9 ms of 80). No generation change; KI-381.

### H-04: the records

The content (ideas note section 9): the genres of each age, from a tablet's line to a chronicle's
year (30 to 400 words), dozens of inputs (the world, the people, the history, the record, the other
people, the language), perspectives (the annal, the letter and the song of one event), the collapse
told from many sides, the register kept (mostly ordinary life; the catastrophe late), every place and
figure named from the plan and the history. The decoder pages long texts; C-06's language share
re-tuned for the larger vocabulary; C-05's voice reads the opening and the rest on request.

* Done when: fifty records of one people per age printed for reading; the lengths by genre; variety
  measured against C-02's (distinct sentences, rule reuse); no anachronism and no unknown place or
  figure (unit); a long text paged on the decoder (frames); the user reads them.

**Done (2026-10-10):** `galaxy/genres.*` and the 28 files of `src/galaxy/grammar/` (`docs/reference/12-civilisations.md`, "The records
(H-04)"; `10-decisions.md`, H-04):

* the genres of each age, 63 in 81 alternatives at the root, by the age of the record's year and its medium and by what the history offers
  at its date; the collapse's witnesses (the warning, the denial, the orders, the inventory, the emergency, the whoever-comes) only in its
  years, the last recording where the history ended; from a tablet's seven words to a chronicle's 257;
* the record model (the world, the people, the record's year from the history, the inventions and institutions at the year, the genome's
  traits; the writer, the reader, the subject, the event, the ruler, kin) and its directives (`@cast`, `@news`, `@entries`, `@years`,
  `@death`, `@deed`, `@life`, `@event`), the salience that tells one event from several sides, every name the history's at the year, the
  lexicon by invention;
* the texts built on a thread, the language by roots (C-06's share re-tuned: all at the people's full count), the decoder's pages (the
  voice reads the page shown, the opening first, the rest on `ENTER`), the radar's opening.

**Reworked (2026-10-10), on the user's reading:** 5% at a hundred texts, nothing repeating, each civilisation its own (`10-decisions.md`,
"H-04, the second pass"): a tongue for every people (`galaxy/tongue.*`: every name its own), the engine's frame memory, fatigue, a
people's own few and their own words, the culture's pools and the ruins in the texts, the everyday in the history (fourteen public kinds,
seven of the families), the grammar rewritten by seven writers (94 files, 16,808 alternatives). The repeated frames 0.2%, 0.9%, 0.6% (at
50, 100, over all; H-04's 10.6%, 34.6%, 15.8%), 17.1% of a people's sentences on a frame another people wrote (65.9%).

  `grammar` prints 3,479 texts and 444 captions of 48 peoples with every check at zero and every name the history's; `grammar ages` the
  fifty of one people per age (`shots/tests/records_<age>.txt`), read and mended; `grammar compare` measures C-02's texts of the same
  peoples: 10.6% of the longer sentences on a repeated frame at a people's first 50 (C-02's 20.4%), 34.6% at 100 (36.4%), 15.8% over all
  (27.6%). Unit 263 (the records' checks rewritten); regress unchanged; flow with `shards_long` and its second page; bench within every
  budget. No generation change; KI-368 resolved, KI-381 narrowed, KI-382.

### H-05: more shards, three categories, by age

The catalogue T-04 sized is filled in three categories, each styled by the age (ideas note section
10): texts on the age's media; music with the age's instruments, forms and medium (a cylinder's crackle,
tape hiss; new timbre families in the synth; C-04's tradition gains the age as an axis); star charts
from a painted ceiling to a navigation chart of routes (C-10's charts by age); a record with no sound of
its own played or read by the ship as the computer reconstructs it.

* Done when: the counts of each category per world by age printed; music of each age rendered as WAVs
  with their levels measured (the user listens); a chart of each age drawn on the decoder (frames); the
  decoder's medium per age (frames).

### H-06: the decoder reads a history

The decoder for a larger catalogue: the categories, a long text paged, the timeline as the history's
eras with the shards on it (C-12's timeline grown), and a page that assembles what the shards read so
far tell of the people, the events learnt in order (the explorer's own legends of a people), the age
named there once the records have told it, the last recording closing it.

* Done when: decoder frames of each page for a people of each age; a unit check that the history page
  holds only what the read shards told; the flow reads a world's shards and opens its history page.

**Done (2026-10-10):** `galaxy/legends.*` and `game/history_screen.cpp` (`docs/reference/12-civilisations.md`, "The decoder reads a history
(H-06)"; `10-decisions.md`, H-06): a world's records on three pages (Tab):

* the records: C-06's list with each row's kind as a glyph (a page, a note, a star) and one kind shown on Left/Right, the kinds counted;
* the timeline: the people's eras across the width by their share of its records, each placed and named once a record of it is read (its
  name read through the language), the records held stacked by their years in their kinds' colours, the events learnt as ticks, the axis
  closed by the last recording;
* the history: the age they reached once three of its records are read (or the last recording), the people's ways their records expressed
  (faith, rite, holy place, living, values, the dead, the end), the events the read records told in the history's order under their eras,
  a person named only once a record wrote the name (else "a woman", "a child", "a ruler"), every line read as the records are (a word not
  learnt yet in their tongue), the last recording closing it; Enter on an event opens the record that told it.

  The records' texts are H-05's byte for byte (an epitaph's death and the last recording's end told besides, apart from the salience).
  `decoder pages` draws each age's pages (`shots/tests/decoder_pages_<age>_*.png`) and prints every history page; the unit's seven history
  checks (the page holds the 41 events 21 read records told and none of the 7 only the unread told, no person named without a record, the
  eras, the age not told then told by a third record, the ways, a reading adds what it told, Enter opens the record, one kind on the records
  page); the flow's `shards_eras` and `shards_history`. No generation change; KI-377 and KI-382 narrowed, KI-387 new.

## Phase 8: the player

### C-11: the ship's player and the instruments of the ages

KI-347: the pieces (C-04) and the recordings (C-05) play only on the decoder screen. With flights of a
minute and a half and a radar that hears voices thousands of years old, the ship plays them: the pieces
and recordings held and the recordings heard (`Guide::heard`) listed on the decoder and played on while
the ship flies (a line on the space HUD, stopped by a landing); and C-11's own row, an instrument found
in the ruins, of its people's age (a bone flute, a lute, a gramophone, a synthesiser), that plays a few
notes of its people's scale from the cabin.

* Done when: a piece and a recording play on through a Vimana flight and stop at a landing (unit, the
  flow); an instrument of each age is found, taken and played (frames, WAVs with their levels).

**Done (2026-10-10):** `game/player.cpp`, `galaxy/instruments.*`, `game/instruments_screen.cpp` and `game/instrument_draw.cpp`
(`docs/reference/12-civilisations.md`, "The ship's player and the instruments of the ages (C-11)"; `10-decisions.md`, C-11):

* the ship's player: `TAB` at the decoder's worlds; the pieces and the recordings held and the recordings the radar caught (`AIR:`, their
  opening through the receiver's band) by worlds and years; `ENTER` plays one and the list on, `SPACE` pauses, `BACKSPACE` stops; it plays
  wherever the ship goes with a line on the space HUD and stops at a landing; held, its place kept, while the decoder plays a record of its
  own, the radar listens or the gramophone turns, and taken up where it was (the synth starts a piece at a beat and a speech at a second);
* the instruments: each people's of each age from its ensembles, what a hand carries (a bone flute, a slit drum, a lute, a lyre, a tabor, a
  violin, a music box, a gramophone, an electric guitar, a drum machine, a synthesiser, glass rods, a swarm engine, a ring that sings, a
  deep stone ...: 49 kinds), beside a third of the music records of their age (every copy of the record), one of each in a settlement, a
  glint that pulses twice, `E` to take; played in the cabin's case on the left wall (`facing` 10) a degree a key on its people's scale in
  its people's timbre, drawn from its own numbers (holes, tubes, frets, strings, bars and keys by the scale); the gramophone plays a disc.

Unit 285 (eight C-11 checks; R-404's colour check counts the case) in 3:40 of wall time; the six scenes find, take and play a slit drum, a tabor, a gramophone, a synthesiser, tuned bars and a deep stone; the 18
instruments of the age worlds' own ages peak at 0.13-0.65 (rms 0.034-0.14), the 7 pitched within 2% of their first note; flow 45.6 s (H-06's binary 42.4 s back to back: the Mac was swapping)
(`player`, `cabin_case`; the piece through the flight, stopped at the descent); regress 22 unchanged; bench within every budget but `descent people` (101.6 ms of 80) and `buggy 2x` (15.0 of 14), which H-06's own binary exceeds the same way on the same swapping Mac (104.6 and 15.1 back to back): C-11 like for like within the noise. No generation change; the
music's WAVs and the records' listings identical to H-06's. KI-347 closed, KI-388 new. The last of 1.3.0's twenty.

## Phase 9: the makers

### C-17: the instruments' maker

Added on 2026-10-10 after C-11, on the user's request: "make shape and form of instruments procedural as well -
civilizations are different, numerous, alien - yes, there are industrial or prehistoric societies but it doesn't
mean everyone was carving bones or building gramophones. It should be sort of a constructor that would place
randomly generated instruments".

C-11's instruments were 49 fixed kinds chosen by the age (every prehistoric people's bone flute, every industrial
people's gramophone and violin), each drawn by its own hand-written picture and object. C-17 replaces the kinds
with a constructor: each people has its own way of making (what its world gave it: wood, stalks and gourds where
plants grew, bone, horn, chitin or shell from its animals and its seas, ice in the cold, stone, clay; the metals and
the made materials of the later ages; and its architecture genome's style: its symmetry, its bodies' outline, its
openings' shape for holes, its tops for heads, its ornament and motif, its number, its proportions), and each
instrument is assembled from parts (tubes, outlined bodies, discs, rings, strings, holes, keys, bands, heads) for
the sound it makes (the voice of its people's music it is: an edge-blown pipe, a reed, lips, a plucked or a bowed
string, struck metal or stone, a skin, a whirled blade, a machine), in a plan drawn from the people's leanings
(a straight pipe, a vessel, a raft, a ring of pipes; a necked body, a frame, a harp, a board, a ring of strings;
a row of bars, of bowls, of stones ...). One list of parts is both the object lying in the ruins and the picture
in the cabin's case; its name is what it is made of and how ("A SHELL PIPE", "A STRUNG GOURD"), and the decoder
names the piece's instruments the same.

* Done when: no instrument is a fixed kind of its age (a machine that played the wax cylinders only where the
  people made one, in a form of its own); the materials follow the world and the age (no wood where nothing grew,
  no shell without a sea, no metal before the medieval age); the same voice's instruments differ between peoples
  by a measure, not by hope (the harness's distinctness over many peoples), and a sheet of peoples by ages shows it;
  each age's is found, taken and played as before (frames); the music is unchanged (its WAVs identical).

**Done (2026-10-10):** `galaxy/instruments.*` rewritten, `galaxy/instrument_plans.cpp` new, `game/instrument_draw.cpp` rewritten, `FK_PLATE`
(`docs/reference/12-civilisations.md`, "The instruments' makers (C-17)"; `10-decisions.md`, C-17):

* each people's way of making from its world (its flora's families, its animals and seas, its cold, its volcanoes, its crystal) and its
  architecture genome (outline, holes, heads, ornament, motif, symmetry, number, rhythm, paint, light), three materials and its plans loved;
* the makes: each voice and kit piece that is a thing, its plan among 97 by its class, its materials, its name (the timbre's too: the
  decoder names the piece's instruments as the ruins hold them); the cylinders' machine where the people made one;
* the constructor: parts by the plan, the scale and the style, the ruins' object and the case's picture one list.

`instruments distinct 100`: 5.0% of the pairs share the plan and the body, 0.87% have pictures alike; prehistoric pipes of bone 13 of 100,
a cylinders' machine 62 of 100 (C-11: every one each); 326 names. Unit 288 (three C-17 checks) in 3:31; flow 34.1 s; regress 22
unchanged; bench within every budget. The music's WAVs and the records' places as C-11's. KI-389 new. The twenty-first of 1.3.0.

### C-18: the drone bay

Added on 2026-10-11 after the bugs B-411..B-421 and R-412 (committed as `6139a2f` the same day), on the user's request: "add three drones
with robotic hands (don't model them exactly like you modeled surface drone but still in similar design) that you can send from stardrifter
to felisian/desert planets with civilizations. When you send them each can bring 2-8 items on stardrifter each (so minimum 6 and max 32),
cooldown is 5 minutes. They can bring both shards (all three kinds) and instruments. As for instruments there is another change I would like
you to add - make it so that instruments are also separated by planet/world as shards are so you open specific planet as a folder (as we
have in shards) and can view all the instruments from the planet".

Three drones at two to eight finds each make six to twenty-four a trip; the request's "max 32" would need eleven a drone, so the per-drone
rule was kept and the difference reported to the user.

* Done when: the cabin holds the three drones (the surface drone's family, not its copy: ducted fans, the dark band of an eye, a beacon;
  two arms with three-fingered hands), seen leaving and coming back; parked at a people's world (felisian or desert, `TR_CIVILISATION`) the
  bay sends them and each brings two to eight of the world's records (texts, music, star charts) and instruments the explorer does not
  hold, into the guide (the decoder, the case), gone from the ruins as one taken by hand is; five minutes from one launch to the next; the
  case opens on its worlds, a world's instruments inside it; checked in the unit and seen in frames.

**Done (2026-10-11):** `galaxy/drones.*` (what a world holds for them and a trip's haul), `game/drones.cpp` (the bay: the trip, the
cooldown, the screen), `game/drone_draw.cpp` (the model, the rack, the drones outside the windows, the screen's lines, the map), the case's
worlds in `game/instruments_screen.cpp` (`docs/reference/12-civilisations.md`, "The drone bay (C-18)"; `10-decisions.md`, C-18):

* the bay: a rack of three shelves against the cabin's left wall by the front window (`facing` 11, pale yellow, the flight computer's
  devices page `THE DRONE BAY`), a port behind each shelf the drone backs through into the hull; the drones fly past the windows and
  dwindle to strobes over the world; the screen (`GameState::DRONES`): each drone in lines (turning on its shelf, its face three-quarters
  on while out, its arms reaching and its hands closing on each find), the world's map with the ground under the ship, the places searched
  and the drones on it, what the world still holds for them, the finds as they are made;
* a trip: each drone two to eight, to a place by how much of its own it holds, three to five there (its own, then copies of its people's
  records kept there), on to a second or a third; the rarer kinds leaned toward; aboard at the dock; real time (the warp does not hurry it),
  a minute or two, five minutes from launch to launch; called back with what was found when the ship leaves; saved and resumed;
* the case: its worlds as folders (the count, whose they are, the world under the ship first), Enter opens one, Esc closes it.

Two hundred hauls from the modern age world (250 items): 7 to 24 finds, 17% instruments (3.6% of the items), 9.5% star charts. Unit 310
(nine C-18 checks) 0 failures; flow 36.6 s with the bay at its end; regress 22 unchanged; bench within every budget. KI-399 new. The
twenty-second of 1.3.0.

**Reworked on the user's review (2026-10-11, the same day):** "Why on my save Bubas doesn't show shards adding up from drones rack? Nothing
changes and rack shows there are about 200+ shards and shards decoder there are 130+ or something. What I am expecting - when drones arrive
everything immediately appears on shard decoder [...] make them look for stuff longer, maybe for 2 minutes + 5 minutes recharge". Every
find was in their guide (Bubas: 58 shards, 35 of them brought by the drones); the rack counted what was left (204) and the decoder each
people's whole (129 and 133), the held split over its two rows; and a decoder left open on a world did not list what docked. Now the rack
says `ABOARD: 58 OF ITS 262 SHARDS` and `AND 7 OF ITS 9 INSTRUMENTS`; the decoder lists again as the guide grows (its worlds, the open
world's records, the player's page) and its rows that ran off the screen with three-figure counts fit; each drone searches for two minutes (`DRONE_SEARCH_S`) and the bay charges five minutes from their
return (`BACK IN 1:52` while they are out). Unit 311 (ten C-18 checks: the charge from the return, two minutes a drone,
a decoder open as they dock) 0 failures; flow 38.2 s; regress 22 unchanged; bench within every budget.

**The case a folder each people (2026-10-11, the same day):** asked whether the case kept two peoples' instruments apart, the user: "in
instruments case split instruments by civilization as a decoder does". A world of two peoples is a folder each people in the case, the
people named on its line as the decoder's rows name them (`UNKNOWN (+208 +0 +51 / 5)  THE VASANTHUL`), the open folder that people's
alone. With it R-413 (the two peoples of a world of one age), the same message's other answer.

**The rack's list and the case's line pared (R-414, 2026-10-11):** the user, of the rack: "all the findings should be cleared after you
took a look at them [...] So drones are back, you open, see what they found, close rack and that's it"; of the case: "On the right you
again see that it's 'From <planet>' and coordinates. No need for that". The rack's screen closed after the drones are back aboard clears
the trip's finds and the places searched (`droneFindsSeen`; a trip still on keeps them); an instrument's line is its name alone, the open
folder's title naming the world and its people on a world of one too. A check: the rack lists none once seen.

## Working rules for this iteration

* One milestone at a time, in the order above, and stop after each for the user's review and
  "proceed". The user may interleave bug reports and requests; handle those as B-4NN/R-4NN.
* Start each milestone by reading its sources, `docs/reference/10-decisions.md` and the reference
  notes it touches. The decisions above are not asked again; a new design question that is the user's
  is asked before building.
* Per milestone: `make -j8`, `unit`, `flow`, `regress`, `bench check` alone, plus the milestone's own
  checks; look at the frames in `shots/tests/`, measure WAVs before judging them, and read generated
  texts by the fifty before judging them. Phase 1 blesses nothing; from A-01 the regress is blessed only
  for a deliberate change of what a hashed frame shows.
* The peoples: one generation change (13) from A-01 for the whole expansion; the civilisation scenes
  and test worlds found again (one per age near home, printed and pinned) and the scene finder's
  exclusion lists kept whole for every new scene code; `12-civilisations.md` rewritten as the blocks
  go; experiments in a scratch build, removed once their decision is recorded.
* Docs per milestone: a dated bullet in `docs/iteration-3/plans/PROGRESS.md`, the reference notes it
  changes (04 galaxy, 06 surface, 02 rendering, 07 controls, 08 testing, 12 civilisations),
  `10-decisions.md`, `KNOWN-ISSUES.md` (KI-349 and KI-364 narrowed, KI-345, KI-347, KI-350, KI-352 and
  KI-353 closed or rewritten by the peoples, new rows for what stays), the status here and in
  `MILESTONES.md`, the request's status line.
* Nothing is committed unless the user asks; commits carry a randomised author.
* The release, when the last milestone is done and the user calls it: the version strings
  (`CMakeLists.txt`, `packaging/make_app.sh`), the README's paragraph and keys, 1.2.0's recipe on the
  merged tree.
