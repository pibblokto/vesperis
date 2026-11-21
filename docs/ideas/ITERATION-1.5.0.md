# Iteration 1.5.0: the small and the spacious

Planned 2026-11-16 on the user's request, on the branch `release/v1.5.0` (from `main` bb0e58b, 1.4.0's merge). Nothing built.
The design and its lists are `IDEAS-small-and-spacious.md`; this file is the milestones, their checks and their order. The user
starts each milestone with "proceed".

**The living small** (`R-425`): microorganisms on every world: "search for and investigate microorganisms that can be found on
any planet", with "complex procedural look generation and in game animations when you bring them to the ship and investigate",
"on different stages", and "interesting mechanics for what you can do with them on the ship". Every landable world gets a biota:
the stage its life reached (from protocells to colonies, the spores and fossils of life that ended), one of four chemistries, the
genesis it descends from (kin across worlds and stars), and its species, each with a niche, a way to live, limits, a life cycle and a
form of about forty values seen six ways. The explorer finds them by choosing where life would be (a sample's microhabitat, its
activity read on the spot, the lamp's ultraviolet, the signs the ground shows) and studies them in a laboratory aboard: the
microscope, the incubator's cultures and their stages, waking spores, mixed flasks, strains, the sequencer and the tree of kin, and
the organisms a dead people made. (Z-05's jars were taken out by R-447, 2026-11-20.)

**The interior** (`R-426`): the Stardrifter "way way more spacious"; a player called it utilitarian. The 5 x 5 m room becomes three
decks and eight spaces, about 820 square metres: the bridge with its great window, the two-deck hall with the garden and the spiral
stair, the library, the laboratory, the quarters, the hangar with the capsule, the engine room and the observatory under a glass
dome; materials, lamps, the star's light through the windows, the hull seen outside, sound by room, and the explorer's things.

**The seeded worlds** (`R-448`, Phase 6): a gene-edited culture sent down to a world changes it, "sometimes beneficial [...] sometimes
neutral, sometimes harmful", more often harmful; fourteen new genes, nuanced ("more co2 which will transform planet into venusian type of
planet"); made traits, some never found in the wild; die-offs with remains of the world's own animals, mutations, blights; worlds
overgrown with flesh, fungus or other tissue, ruins included; and the animals drawn again with far more variety, still procedural.

**Generation.** One change, 17, from Z-01: the biota is a new part of what a body is, drawn on salts of its own after every draw of
16, so no star, system, body, map or ground moves with it. Z-03's two signs on the ground (the hot springs' bands, red snow) ride
it and move only the worlds they are given. The I series is drawing alone: no generation change, `cabin_1x` blessed where named.
Saves of 1.4.x load with the regeneration notice. Phase 6 makes one more, 18, from J-06: the animals drawn again. Seeding is no
generation change: what the explorer did to a world is kept in the guide, and a world nobody seeded stays as generated.

Left out on purpose: caves (the K series, still unplanned: no cave life), life in a giant's clouds (X-03's rule), drones that sample
(the search is the explorer's), plants and animals as specimens (only the small comes aboard), the V and L series. Seeding a world
with a culture, left out at first (the worlds stayed as generated), came in with R-448 (Phase 6).

## Order

| # | Phase | Id | Item | Size | Status |
| --- | --- | --- | --- | --- | --- |
| 1 | The living small | Z-01 | Where life is: each world's stage, chemistry, genesis and species; the kin cells; generation 17 | L | done 2026-11-16 |
| 2 | The living small | Z-02 | What it looks like: the form genome, six ways of seeing, the life cycle's animations; plates to judge | XL | done 2026-11-16 |
| 3 | The ship | I-01 | The ship as rows: rooms, openings, fixtures and devices; the harness reads them; nothing drawn changes | L | done 2026-11-16 |
| 4 | The ship | I-02 | The new Stardrifter: three decks, eight spaces, the stair and the arches, the devices in their rooms | XL | done 2026-11-16 |
| 5 | The ship | I-03 | Light, materials and views: lamps, the star's light through the windows, the hull seen outside | L | done 2026-11-16 |
| 6 | The ship | I-04 | Life aboard: the quarters, the garden, the library's shelves, the hangar's capsule, sound by room | L | done 2026-11-17 |
| 7 | The search | Z-03 | The search: microhabitats, the kit, the activity, the lamp's ultraviolet, the signs, the capsule's scoop | XL | done 2026-11-17 |
| 8 | The lab | Z-04 | The microscope: a camera you enter, six ways of seeing, zoom, tracking, light and field; names and sheets | XL | done 2026-11-17 |
| 9 | The lab | Z-05 | The cultures: the incubator, growth and the stages, waking spores, mixed flasks, strains, the jars (taken out by R-447) | XL | done 2026-11-17 |
| 10 | The lab | Z-06 | The genome and the tree: the sequencer, kin across worlds and stars, the geneses, the made organisms | L | done 2026-11-17 |
| 11 | The close | Z-07 | The record: the catalogue, the LIFE page, lending, auto-explore's samples, the docs and the battery | M | done 2026-11-17 |
| 12 | The genes | Y-01 | The trees by world and by folder: a page a world and a folder, the lab's lines as twigs (R-444) | M | done 2026-11-19 |
| 13 | The genes | Y-02 | The gene editor: a species' own genes cut, changed by hand, put into a flask, learned (R-443) | XL | done 2026-11-20 |
| 14 | The genes | Y-03 | Genes from others: moved from kin read and cultures kept, their price, duplicates, needs (R-443) | L | done 2026-11-20 |
| 15 | The genes | Y-04 | The switches: when a gene is on, in the culture's segments and the drop (R-443) | L | done 2026-11-20 |
| 16 | The seeded worlds | J-01 | World genes: fourteen new genes on every species, seen in the lab; the editor's second page | XL | done 2026-11-20 |
| 17 | The seeded worlds | J-02 | The world now: a seeded world's layer over the world as generated; nothing drawn changes | L | done 2026-11-20 |
| 18 | The seeded worlds | J-03 | Sending: the drones' seed trip, whether it takes, the spread, the record; the waters' colours | XL | done 2026-11-21 |
| 19 | The seeded worlds | J-04 | Air and sky: the made climate, haze, ozone, clouds, rain | L | done 2026-11-21 |
| 20 | The seeded worlds | J-05 | Water, ground and plants: seas, snow, crusts, mats, blight, galls, new leaves | XL | done 2026-11-21 |
| 21 | The animals | J-06 | Animal bodies: parts, sixteen plans, a world's style, hides of their own; generation 18; plates to judge | XL | done 2026-11-21 |
| 22 | The animals | J-07 | Animal ways and the record: gaits, hunters, swimmers, flocks; animals named by the explorer | L | done 2026-11-21 |
| 23 | The animals | J-08 | Fates: die-offs and remains, sickness, changed young, thriving, silence | XL | planned |
| 24 | The overgrowth | J-09 | The overgrowth: flesh, fungus and eight more over the ground, with structures; plates to judge | XL | planned |
| 25 | The overgrowth | J-10 | Ruins overgrown, and the sounds of an overgrown world | L | planned |
| 26 | The close | J-11 | Made kinds: hothouse, snowball, haze, purple, rust, overgrown, charnel, greened | M | planned |
| 27 | The close | J-12 | The record and the close: catalogue, LIFE page, docs, battery | M | planned |

Each row's `Status` moves to `done` (with the date) when its milestone is finished, and the matching row of `MILESTONES.md` gets
`1.5.0, done <date>`. This table is the order.

## Why this order

1. **The forms before any screen.** The look is the request's heart and its riskiest part, so Z-02 ends in plates the user judges
   before the microscope is built on them (C-02's "judge the text before any audio"). The biota comes first because a form is its
   species': its plan follows the stage, its pigment the way it lives, its style the genesis.
2. **The ship before the lab.** The lab is a room of the new ship. I-01 proves the engine on the old room bit for bit (P-00's and the
   U series' discipline), so I-02 is rows and fixtures, not plumbing, and a difference in it is content, never an engine's bug.
3. **The ship whole before the microbes come aboard.** I-03 and I-04 answer "utilitarian" (I-02 alone is a bigger box), and the
   lab's devices then arrive in a ship that is already a home.
4. **The lab in the order a sample meets it:** found (Z-03), seen (Z-04), grown (Z-05), read (Z-06).
5. **Could go otherwise:** the I series first (the ship whole before any microbe), or Z-03 straight after Z-02, the vials counted
   into a cold store with no lab until Z-04. Either is one word from the user.

## Decided by default (2026-11-16)

The plan's defaults stand unless the user changes them; a milestone starts on "proceed" without asking them again (a new design
question that is the user's is still asked before building):

1. **One release:** all eleven are 1.5.0, in the order above.
2. **"Stages" both ways:** the stage of a world's life (sterile, prebiotic, simple, complex, colonial; dormant and fossil where life
   ended) and the stages of an organism's life (dormant, waking, growing, dividing, a colony, gathering and fruiting, sporulating,
   dying), which the lab shows and the explorer brings about.
3. **"Any planet":** every landable kind gives a sample something to show but the furnaces (magma ocean, molten, chthonian, steam,
   glass, the lava worlds), whose samples hold grains and at most cooked organics; none in a giant's clouds (X-03), and no life fed
   by light without a sun (a drifter: chemotrophs and spores).
4. **Four chemistries:** water, acid, ammonia, hydrocarbon; the three proposals' sheets say what is seen, never that it is so.
5. **Kin:** most systems a genesis of their own; a system's living worlds one; one living system in five its 64-sector cell's oldest;
   a drifter its origin star's; home's genesis pinned, the ship's own species of it at the tree's root.
6. **Nothing is named until the explorer names it,** and no count is hinted: a world's total of species is never shown, the scanner
   gives no sight for life, the sample's activity and the lamp are the only hints.
7. **The kit:** eight vials a descent, refilled aboard; the cold store holds 64 (full, a sample is refused on the ground with its
   line); a sample is a pure function of its square metre.
8. **The lab's time** is the drones' clock (real seconds while the game simulates, saved; nothing while it is closed): a culture
   reaches its stationary phase in 8-20 minutes at its best, a spore wakes in 20-60 seconds, a sequence takes about 90.
9. **The ship:** IDEAS 2.2's three decks and eight spaces; the old room goes (one code path); the classic cockpit camera stays the
   SHIP VIEW setting; nothing needs walking (every device keeps its key and its menu row).
10. **Walking:** 1.6 m/s, Shift 3.2 m/s; the spiral stair and the bridge's steps; open arches, no doors to work.
11. **The microscope is a camera you enter** (rules-game-devices.md): the slide in the framebuffer with every bank its own, the
    HUD on the canvas; the other lab screens are screens over the room, as the decoder is.
12. **The made organisms are in** (Z-06): one species on each world of a futuristic or far-future people, its watermark a record
    in the people's tongue. Left out by one word, the rest of Z-06 unchanged.
13. **Lending:** species, names and sheets travel (cyan, in no statistic); samples and flasks do not.
14. **Auto-explore** takes samples at water and springs on its walks; the lab's work stays the explorer's (it opens no screen).

## Phase 1: the living small

Source: `IDEAS-small-and-spacious.md` sections 0 and 1; the reading of 2026-11-16 (the kinds' life tags, Q-02's life and water
traits, the materials, the ground's finders, `drifterPast`, the protected worlds); `docs/reference/04-galaxy-and-systems.md`,
`05-planet-function.md`; the `world-kind` skill.

### Z-01: where life is (generation 17)

* **A new directory, `src/life/`,** reading `src/galaxy` and `src/core`: its CLAUDE.md, its layer in `tools/check.py`.
* **The table:** a row per kind (`life/biota`), checked at compile time as `KINDS` is (every kind a row; a landable kind's stages
  never empty but a furnace's). A row holds:
  * the stages the kind can hold, with their weights;
  * its chemistry and its species' range;
  * its niches, from the microhabitats it has (a sea, drainage, springs, ice, salt, snow, a plume, ruins, flora's soil).

  A dead row (frozen and scorched felisian, rust) takes its `thenKind`'s life as spores and its old seabeds' as fossils; the dark
  kinds take chemotrophs and spores only; a giant none.
* **The draw:** `biotaOf(sys, body)`, on salts of its own after every draw of 16, computed on request (as `drifterPast`) and cached
  by system.
  * The stage is picked among the row's by a hash and the world's facts: a world with flora COLONIAL; an ocean or eyeball world
    COMPLEX or COLONIAL; an archean SIMPLE, one in five COMPLEX; a dead world DORMANT with FOSSIL; a comet PREBIOTIC; a hydrocarbon
    world PREBIOTIC, one in six SIMPLE.
  * The species, 3-16 by stage (spores 1-4), each with its niche, its way to live, its limits, its life cycle, its behaviour, and the
    seed its form grows from (Z-02).
* **The geneses** (`life/lineages`): cells of 64 sectors, each with a genesis or two by its own salt.
  * A living system's genesis is its own or, one in five, its cell's oldest; a system's living worlds share one.
  * A living world's airless and icy neighbours keep its spores: 1-2 species, dormant, in shade and ice.
  * A drifter's life is its origin star's (`drifterPast`).
  * A genesis's handedness, alphabet and style come from its hash.
  * Home's genesis is pinned (Heilya II's; a pinned salt), and the ship's own three species are of it.
* **Generation 17:** `GEN_VERSION` 17, its row and section atop `generations.md` (rules-galaxy.md: a change to what a body is bumps,
  regress moved or not). No draw of 16 moves: no regress hash, no `fingerprint bodies` line.
* Done when:
  * `vesperis_test life list [n]` prints the biota of the n nearest worlds of home: stage, chemistry, genesis, and the species with
    their niches and limits;
  * `life survey 1500` prints the shares by kind and stage: something on every landable kind but the furnaces, a living world's
    species 3-16, the dead worlds' spores, the airless neighbours' spores near home;
  * `life kin [ly]` prints the geneses within 200 ly of home, the kin systems by cell, and the drifters near home with the star their
    life came from;
  * a unit group passes: the biota deterministic and the same at any thread count; every landable kind a row; the furnaces sterile;
    no species fed by light on a sunless world; no giant with a biota; home's genesis pinned; a living world's neighbours' spores of
    its genesis; a system's two living worlds of one genesis;
  * `fingerprint life` (keyed lines over the four regions and the drifters) joins the battery;
  * `fingerprint bodies` and regress 24/24 are unchanged both ways; `bench check` holds all 38 budgets.

### Z-02: what it looks like

* **The form genome** (`life/morphs`): a species' form from its plan (by stage and chemistry), its forty values in their groups
  (IDEAS 1.5) and its genesis's style.
  * The forms of its life cycle: the spore, the cyst, the dividing cell, the colony, the gathered slug and its stalk, the ghost
    after lysis.
  * `morphDistance` over the strongest values, as A-04's genome distance (colours by distance, measures by their spread,
    categories 0 or 1), with an alike threshold set from the measure.
* **The drawing** (`life/morph_draw`): a form in a way of seeing at a time, into a framebuffer of its own (every bank the slide's,
  the ramps the way of seeing's).
  * The outline from the superformula or a capsule, shaded as relief from its edge; the inside's parts; the appendages as
    travelling waves.
  * Anything thin is sized in screen pixels: a lone pixel dies in the mush.
  * The electron's relief from a mesh of the cell lit from one side; the section's minerals in their interference colours.
* **The animations** of IDEAS 1.6, each closed-form in the slide's clock from the species' values and a seed:
  * motion and beating;
  * division, budding, the colony's release;
  * sporulation and lysis, waking, the cyst;
  * engulfing, bridges, the turns to light and field, the glow;
  * the gathering's spiral waves and the stalk, the colony's branching front, a protocell's thread.
* Done when:
  * `life plates [n]` writes a plate for each of the n nearest worlds of home with life (`shots/tests/life_plate_<n>.png`: its
    species in the ways of seeing that suit them, at their sizes) and one for each stage and chemistry (`life_plates_<stage>.png`);
  * `life strips [plan]` writes each animation in eight frames over its time;
  * `life distinct` finds no two species of a world alike by `morphDistance`, and under 1.5% of pairs alike among 1,000 species by
    96-px thumbnails (a mean RGB difference under 6, C-17's measure); it draws the 16 nearest pairs side by side;
  * unit checks pass: a slide's frame at a time is the same reached by skipping and at any thread count; a form stays within its
    bounds at every zoom; a way of seeing's banks stay within 32;
  * the plates are the user's to judge before Z-04 builds the microscope on them;
  * nothing of the game changes: regress, `screens` and the tour as Z-01's.

## Phase 2: the ship

Source: `IDEAS-small-and-spacious.md` section 2; the reading of 2026-11-16 (the cabin's pieces and bands, its banks, its hand
coordinates in the harness and auto-explore, the bench rows that draw it); `docs/reference/02-rendering.md`,
`07-game-flow-and-controls.md`; rules-game-screens.md, rules-rendering*.md.

### I-01: the ship as rows (nothing drawn changes)

* **The rows** (`game/ship_plan`): rooms, openings, fixtures and devices (`cabinDevices` gains a room and keeps its order).
  * The 1.4.0 cabin is the first plan, piece for piece in its serial order: the floor, the ceiling's ring, the walls and mullions,
    the cage's bars, the capsule, the consoles, the case, the rack, the desk, the right wall's screens; the bulb and the orb after
    the bands.
  * The roof deck is a room reached by the lifter.
* **The mesh** is built once in ship metres with each piece's shade factors; the frame's shades (`hullShade`, the flicker, the
  screens' states) apply as today, each float expression keeping its shape.
* **The frame:** each polygon is projected once a frame before the bands, and each band fills only those that reach its rows
  (`CabinFrame` carries them: what bands share is made first and only read). The banks are dealt from the rows; this plan's numbers
  are today's.
* **The walk** comes from the plan: the walls and the fixtures' footprints are the colliders (`keepOff` and the cage's circle become
  rows), and facing goes by room.
* **No hand coordinates left.** These read the rows instead:
  * the tour's `DEVICES` list, the flow's cabin steps, `screens`' cabin views;
  * the unit's cabin checks (unit_sky, unit_navigation, unit_civ, unit_peoples, unit_proof) and the bench's decoder row;
  * auto-explore's `CAB_WINDOW_X/Z` and its walk round the cage;
  * the rack's eye height and place in `drone_draw`.

  R-404's colour check reads the table's count, not a literal ten.
* **The save:** the `cabin` line is kept; the room is a line of its own (`cabin_room`), read as the plan's first when absent.
* Done when:
  * every picture, sound and hash is the same: `tools/prove compare` against the base is identical (regress 24/24 both ways,
    `screens` 96/96, the tour's 410 pictures and 81 texts, the flow's 164, the unit's);
  * the harness's change is proven on the base's engine objects (`PROVE_BIN`);
  * `bench check` holds its budgets, and AB/BA pairs of space 2x, space 4x, the arrival and the decoder are within the noise;
  * the cookbook's "Add a cabin device" is a row and its fixture.

### I-02: the new Stardrifter

* **The plan:** IDEAS 2.2's eight spaces on three decks as rows.
  * The spaces: the bridge, the hall, the library, the laboratory, the quarters, the hangar, the engine room, the observatory.
  * The ways between them: the spiral stair, the bridge's steps, the arches.
  * The hull: the outer walls, their windows and the dome.

  The old room goes (one code path): nothing of it stays beside the new.
* **The devices in their rooms:**
  * the bridge: the flight desk, the star map, landing and target screens, the GOES console;
  * the library: the decoder and the case;
  * the observatory: the telescope and the radar;
  * the hangar: the capsule, the drone rack with its door, the probe's launcher (the landing screen still launches a probe at a
    giant).

  The lab's benches, shelves and the cold store's door, and the quarters' bunk and desk, stand as fixtures, waiting for Z-04 to Z-06
  and I-04. Each device keeps its colour; the lab's new devices get theirs when they arrive.
* **The walk:** the stairs' heights by place, Shift 3.2 m/s, the arches' widths, the hatch closed.
  * A new game starts on the bridge facing the window, and the climb aboard returns the explorer where they left from.
  * An older save's `cabin` line, of the 5 m room, is read as the bridge's start.
* **Drawing:**
  * the portals: the room stood in, and the rooms seen through its arches and stair, depth-limited;
  * the space view drawn only when a window, the oculus or the dome is in view, else skipped, with every bank dealt;
  * the banks dealt by nearness.
* **Auto-explore:** its window is the bridge's, its roof deck the observatory, and its walks are routes through the arches (a graph
  of the plan's openings).
* Done when:
  * `vesperis_test ship rooms` writes each space from its arch and from its middle at 1x and 4x, printing the banks each frame dealt;
  * `ship walk` walks a route to every device and through every arch, printing the floor's height along the stairs (continuous) and
    every wall kept;
  * unit checks pass: every room reachable, every device in a room and reachable, no collider in an arch, R-404's colours 0.3 apart
    over the table's count;
  * the tour faces every device from its row, none lost; the flow's cabin steps run on the new plan; auto-explore's window and dome
    steps run in the flow's auto run;
  * regress: `cabin_1x` is blessed once and named (the bridge at Heilya II, the light on), every other hash holding both ways;
    `screens`' cabin views are re-pictured, their new names listed;
  * bench: rows for the bridge, the hall, the library, the lab and the observatory at 4x, budgeted at 1.5 times their medians; space
    4x (the start, on the bridge) within its 9.5 ms.

### I-03: light, materials and views

* **Materials:** warm panels, a darker floor, fabric, brass, plants and glass, their banks dealt by the frame (the hull and the light
  fixed).
* **Lamps:** each room's lamps as pools, by per-vertex light on subdivided floors and walls, fixed with the mesh, with the flicker
  and U's dimming applied by frame. A lamp's glow on a wall is a shaded pool, never `glowDisc` (which lands on empty pixels only).
* **The star's light:**
  * each window in view casts its patch along the sun's direction onto the floor and walls, an added shade in the receiving
    surface's own bank, moving as the ship turns and orbits;
  * the parked world's light, by its phase, falls on the walls that face it;
  * a sunless system casts none.
* **The views:** the hull seen through the windows and the dome (the bow, the flanks, the radar's mast turning while it sweeps, the
  drive's ring aft, the drones' door), lit by the star. Y depolarises every outer wall; the dome stays glass.
* Done when:
  * `ship light` writes each room lit and dark, a window's patch at three sun angles, and the hull from the dome sunlit and sunless;
  * pngdiff measures the patches' movement between frames;
  * the dealing is printed: no frame over 32 banks, no two hues in one bank;
  * regress `cabin_1x` is blessed again and named;
  * the bench rows hold their budgets, the bridge facing the sun among them.

### I-04: life aboard

* **The quarters:**
  * the bunk: E ("REST UNTIL ...") opens the almanac to run the clock to an event;
  * the desk: E opens the log;
  * the wall: the gallery's last twelve photographs, printed in one tone (a bank's ramp by luminance), the newest first.
* **The hall's garden:** the home world's flora (its `TreeLook`, its two colours) in beds, swaying in the air system's draught and
  grown by the days played; the lamps; the Vimana's core under the floor glass, brightening with a flight.
* **The library:** the shards on shelves, lit by how many are held of each world's; the instruments behind the case's glass; the
  player's deck (E opens the ship's player).
* **The hangar:**
  * the capsule as the surface draws it, in its cradle;
  * its hatch opening as it drops: boarding at the capsule shows the drop from the hangar before the descent's view;
  * the drones on their rack by their door, leaving through it; the kit's locker.
* **Sound by room,** in levels (the synth is mono), read from renders (`tools/wav.py`), never by ear:
  * the hum by room, the engine room's loudest, and the drive rising in a flight;
  * the lab's bubbling, the garden's water, the dome's quiet;
  * the player's music fuller in the library.
* Done when:
  * frames of each: the quarters by night with its photographs, the garden at two times played, the shelves full and empty, the
    hatch's drop;
  * `sounds` renders each room within the mix's rules (rms and peaks);
  * the tour and the flow run with the new acts;
  * the bench holds its budgets, and regress `cabin_1x` is unchanged or blessed with its reason.

## Phase 3: the search and the lab

Source: `IDEAS-small-and-spacious.md` sections 1.7 to 1.9; the reading of 2026-11-16 (the keys on foot, the lamp of D-04, bank 15,
the trait banks, the hot springs, the descent, the drones' clock, the guide's keys and lines, the statistics page).

### Z-03: the search

* **The microhabitat** at the crosshair within 2 m (`surface/sampling`), read from the site's material, water, heat, ice, snow, salt,
  shade, ruins, flora and plumes. The sample's species and their counts come from it and the biota, a pure function of the square
  metre.
* **The kit:**
  * eight vials a descent, on a key of its own, checked against every binding in every state;
  * the take's HUD line: the sample's name, its temperature, its activity (NONE, FAINT, STRONG);
  * refused from a vehicle and when the vials are used up;
  * at the climb aboard the vials go to the cold store ("6 SAMPLES TO THE COLD STORE"); its 64 places are the save's lines.
* **The lamp's ultraviolet** (Shift+U on foot, checked): within the lamp's reach, at night or in shade, living films fluoresce by the
  biota's density. It is drawn in bank 15, lent as the chemotroph's spores and the milky sea's light are, with nothing where nothing
  lives; it adds after the old result and is absent to the bit when off.
* **The signs:**
  * the existing ones are read as their microhabitats;
  * the hot springs' mats lie in bands by temperature, the hottest bare;
  * red snow is a life trait (`TR_SNOW_ALGAE`, appended) of cold living worlds without a people, given by its own hash in a trait
    bank (Q-02's protocol: `salt scan`, the protected worlds, `fingerprint bodies`).
* **The capsule's scoop:** on a venusian or acidic world's descent, at the clouds' temperate height: one vial and its line.
* Done when:
  * `life search <scene>` prints each microhabitat of the test scenes and what a sample there holds, the same twice;
  * frames: the lamp's glow at night on mats, a shore and a stromatolite field; a spring's bands; red snow; the kit's line and the
    scoop's; `traits SNOW ALGAE`;
  * unit checks pass: a sample a pure function of its place; the kit's count; a sterile world's samples holding nothing alive; the
    lamp's pass absent to the bit when off;
  * the flow takes three samples and carries them aboard;
  * `fingerprint bodies` moves only the worlds given red snow or banded springs; regress blesses the frames that show them, each
    named;
  * bench: forest 4x and the descent's first frame within their budgets.

### Z-04: the microscope

* **The device:** the lab's first, its row in a colour of its own; E opens it on the cold store's vials and the flasks. It is a
  camera you enter: the slide drawn by `morph_draw` in the framebuffer with every bank the slide's, the HUD on the canvas, neither
  the space view nor the cabin drawn.
* **The keys,** each checked against every binding:
  * 1-6 the ways of seeing; zoom, focus and pan; Enter tracks the cell under the crosshair;
  * the spot of light, the magnet, the warm stage, the drop of food;
  * P a micrograph to the gallery; N names the species under the crosshair (`TEXT_ENTRY`, a `TextKind` of its own); Tab its sheet.
* **The slide:** a vial's sample as found (its stage: active, dormant, or fossil in SECTION) or a flask's culture as it is (Z-05); a
  few hundred cells at most; its clock closed-form, with a skip hook for the harness. The list of vials empties one on a key, freeing
  its place in the cold store (the store itself a fixture, its door lit by how full it is).
* **The record:** the first sight of a form (the HUD's line, the log's FORM); the sheet's lines by what was seen (IDEAS 1.9); the
  guide's `M` keys and its `microbes 1` line, byte-stable.
* Done when:
  * `lab microscope` writes frames of every way of seeing at every zoom, on samples of each stage and chemistry, with a tracked cell,
    the light's crowd, the magnet's turn and a fossil's section;
  * the tour opens the state, with its keys and its controller page;
  * unit checks pass: a slide's frame closed-form in its clock and the same at any thread count; a sheet's lines only by what was
    seen; the guide's save byte-stable;
  * the flow looks at its sample;
  * bench: a microscope row at 4x on the densest slide, budgeted at 1.5 times its median.

### Z-05: the cultures

* **The incubator:** twelve flasks, a medium each (temperature, light, energy, salt; the cryostat, the acid cell, the ammonia cell;
  AS ITS WORLD). Its screen shows the flasks' curves on a log scale and their stages.
* **The model** (`life/cultures`): Monod's growth with a lag, a stationary crowd and a decline, closed-form from each flask's events
  on the drones' clock (real seconds while the game simulates, saved).
  * The stages a culture passes are each species' own (waking, growing, dividing, the colony, gathering and fruiting, sporulating,
    encysting, dying), seen on the curve and through the microscope.
  * Dormant spores wake: "WOKEN AFTER ABOUT 12,000 YEARS", the years from the world's end, the drifter's leaving or the spore's own.
  * A luminous species glows when crowded.
  * Two in a flask: one wins, they cycle, they feed each other, or mirrored handedness ignore each other.
  * Strains: a culture held at its range's edge moves one value, the explorer's to name.
* **The jars:** a world's silt in a column on the shelf, its bands settling over twenty minutes. The flasks and the jars light the
  lab and glow in the dark ship, their banks dealt as I-02's.
* Done when:
  * `lab cultures` prints the curves of twenty species in four media, the stages' times, and the table of pairs (the cycles'
    periods, the winners, the mirrored);
  * `lab jars` writes a column at 0, 5, 10 and 20 minutes;
  * unit checks pass: a culture's state closed-form from its events; a save and load round-trip; the clock stopping off the
    simulating states as the drones' does;
  * the flow starts a culture, skips its clock and reads it; the tour runs the screen;
  * bench: the lab at 4x with every flask lit, within its budget.

### Z-06: the genome and the tree

* **The sequencer:** a species from a flask or a vial; about 90 seconds of reading, shown as it reads in its genesis's alphabet; the
  result a line and its place in the tree.
* **The genomes** (`life/sequences`): a sequence is a pure function of the species and its genesis. Distances rank as the lineage
  does: a world's nearest, then a system's, then a cell's, and none across geneses.
* **The tree:**
  * every sequenced species by kinship: clades by genesis, the ancestors' years, the worlds (or UNKNOWN);
  * Enter on a kin's world sets the remote target;
  * the geneses' count; the ship's own species at its root;
  * a drifter's life beside its origin's, once both are sequenced.
* **The made:** on the futuristic and far-future peoples' worlds, one species is theirs (a genome too regular, a watermark the
  decoder reads): a record of a genre of its own in the people's tongue (the `grammar` skill: its shelf, its checks, the pinned
  hash), keyed with the species.
* Done when:
  * `lab tree` prints the tree of the flow's sequenced species, and the kin near home across stars with their years;
  * `life made` prints the made species near home and fifty of their marks as the decoder reads them (about 5% repeats at a
    hundred, as the records');
  * unit checks pass: sequences deterministic; distances ranking as the lineages; no common ancestor across geneses; mirrored species
    ignoring each other in a flask; home's species at the root;
  * the flow sequences a species; the tour runs the screens; the grammar's checks and pinned hash hold.

## Phase 4: the close

### Z-07: the record and the close

* **The catalogue:** a guide page by world and by kin; a species' sheet with its form drawn live; the explorer's names.
* **The statistics:** a second page, LIFE, beside the kinds' page: samples, species, worlds, stages seen, the woken and their years,
  sequenced, geneses, kin across stars; counts in the other screens' words.
* **The rest of the record:** the data sheet's microbe line, the log's kinds (SAMPLE, FORM, WOKEN, KIN, STRAIN), micrographs in the
  gallery.
* **Lending:** a friend's species, names and sheets in cyan, in no statistic; never their samples or flasks.
* **Auto-explore** takes samples at water and springs on its walks; the lab's work stays the explorer's (it opens no screen).
* **The docs:**
  * a reference note for life (`docs/reference/13-life.md`), and the ship in 07;
  * the cookbook's recipes: a kind's biota row, a lab device, a room, a fixture;
  * the rules' lines and KNOWN-ISSUES rows for what stays;
  * the battery's items (`fp_life`, `life`, `lab`, `ship`) and the generated pages.
* Done when:
  * frames of the catalogue and the LIFE page;
  * a lent file's species show in cyan;
  * the flow's auto run takes samples;
  * `tools/prove compare` against the series' last base, every difference named;
  * `bench check` within all budgets, and `make check`;
  * `tools/prove snapshot z07`.

## Phase 5: the genes

Added 2026-11-19 on the user's requests after Z-07, talked through first: R-443 (genes changed: "smart but not boring so that you can
somehow understand and learn what affects what") and R-444 (the trees by world and by folder). The design is `IDEAS-genes.md`. No
generation change: a species' genes are read from what it is, and an unedited line is its species bit for bit. Y-01 first (the smallest;
the strains are the lab's lines already), then the editor on a species' own genes (one organism, "tons of variations"), then genes from
others, then the switches, which act on every gene before them.

Decided by default (2026-11-19), from the sketch and the user's five calls:

1. **The editor is a machine of its own** in the laboratory (`EDITOR`), on the bow bench beside the sequencer where the plan's rows give it
   room; it takes a culture (a flask's or a kept one) whose species the sequencer read, and writes its edit in 120 s on the lab's clock.
2. **An edited line goes into an empty flask** in its source's medium, a new culture the collection keeps as it grows; a line is edited
   again the same way.
3. **Versions:** a gene's working part changed by hand (its genesis's code: a letter's weight, a table's choice) and, from Y-03, a gene
   moved in from a species read or a culture kept of the same genesis.
4. **Learning:** one gene changed and seen against its parent; several, the candidates said; a vital gene's death; a strain read.
5. **Fixed:** how it lives and its plan's family; its values within its cell's range; the worlds as generated.
6. **The trees:** the ship's page, a page a world, a page a folder; no tree of everything (the user's call); the lab's lines on their
   world's page as they arise, Backspace off, A back; a folder's page what it holds.

### Y-01: the trees by world and by folder

* **Pages** (Left and Right): THE SHIP (its own three), each world with a species read or a line of one, each of the collection's
  folders (UNSORTED, then the explorer's); the page's name and its place among them in the head.
* **A world's page:** its species read in the tree's order, those seen and not read counted; its species' lines (strains now) as twigs at
  today's edge under their parents; Backspace takes a line off, A puts one back (the guide keeps which are off).
* **A folder's page:** what it holds (a pair two leaves; a line under its parent where both are in it); those not read counted.
* **Each page its own years;** a leaf's nearest kin read anywhere told under the tree; the room's screen the page last shown.
* Done when:
  * `lab tree` prints every page of a stocked lab (the ship's, its worlds', its folders') with their leaves and twigs;
  * unit checks pass: a world's page holds its species read and no other world's; a folder's what it holds; a strain's twig under its
    parent as it arises, Backspace taking it off and A putting it back, kept over a save and load; a page's axis to its oldest ancestor;
    the nearest kin read anywhere; the LIFE page's geneses count as before;
  * the tour runs the pages; frames of a world's page and a folder's;
  * `tools/prove compare b443`, every difference named; `bench check`; `make check`; `tools/prove snapshot y01`.

### Y-02: the gene editor and a species' own genes

* **The genes** (`life/genes`): a species' genes by what it is (three vital, one to three quiet; warmth, salt, size; pigment, food,
  motion, up to three senses, rest, glow, shape by what it uses), a genesis's map (its numbers) and code (its letters' weights, its
  tables), the wild letters its own values; a line's values from its edits, within its limits.
* **The machine:** its row, fixtures, stand, light and screen (`game/editor.cpp`); its chooser (THE CULTURES, read ones); the map, a
  gene's working part, C, the letters by hand, Backspace; writing in 120 s, into an empty flask; N names a line.
* **A line** read as a strain is: its flask's culture, the microscope's drop, the sequencer (what differs from its parent), the
  collection, the trees (a twig); the guide's `line` and `gene` lines, the save's editor.
* **Learning:** said as seen in the incubator, the microscope and the sequencer (1.4 of the design); the map's words.
* Done when:
  * `lab genes` prints a world's species' genes for worlds of four, six and eight letters and both hands, and an edit's values before
    and after;
  * unit checks pass: genes deterministic; every species of forty worlds unedited is itself (its culture's segments, its slide's kinds);
    each gene's edit moves its value as its code says, within its limits; a vital gene's edit kills, a quiet one shows nothing; how it
    lives and its plan's family never move; a line's flask, drop, reading and keeping; learned from one change, candidates from several,
    a strain read; the editor, the lines and their flasks over a save and load;
  * the machine in the lab (`ship rooms`' laboratory, the tour's stand), frames of its screen; the flow writes an edit into a flask;
  * the battery against y01, every difference named; `bench check`; `make check`; `tools/prove snapshot y02`.

### Y-03: genes from others

* **M moves a gene in** from a species read or a culture kept of its genesis (refused across geneses: another alphabet, the other hand);
  **D duplicates** one; the price by the donor's distance and of power and glow; what a gene needs (a sense without motion, a pigment or
  a food in another way of living); the donor's line on the trees.
* Done when:
  * unit checks pass: a moved gene takes its donor's letters and value; refused across geneses with its words; the price by distance in
    the doubling and two in a flask; a duplicate's place; a sense without motion answers nothing; the donor's line drawn;
  * `lab genes` prints a moved gene and its price;
  * the battery against y02; `bench check`; `make check`; `tools/prove snapshot y03`.

### Y-04: the switches

* **S sets a gene's switch:** ALWAYS, NEVER, IN THE LIGHT, IN THE DARK, WHEN CROWDED, WHEN STARVING, WHEN COLD; the wild ones a glow's
  crowd and a rest's hunger; the culture's segments where its condition changes, the drop as its flask has it; a gene off costs nothing.
* **The close:** 13-life's genes and trees, a `rules-life-genes.md`, KNOWN-ISSUES rows for what stays, the generated pages.
* Done when:
  * unit checks pass: each switch's condition in the culture's segments (the light, the warmth against the line's best, starving, the
    crowd's share) and in a drop; a gene off costs nothing; the wild switches give every species' curve as before;
  * `lab cultures` prints a switched line's curve; frames of a switched glow and rest;
  * the battery against y03, every difference named; `bench check`; `make check`; `tools/prove snapshot y04`.

## Phase 6: the seeded worlds

Added 2026-11-20 on the user's request after Y-04 and R-447 (R-448), talked through first. The design is `IDEAS-seeded-worlds.md`. A
world nobody seeds stays as generated, bit for bit. One generation change, 18, from J-06 (the animals are part of what a surface is);
seeding is the explorer's, kept in the guide, and no generation change.

Why this order:

1. **The genes first:** a world changes by what a line carries, and the explorer learns each gene in a flask before a world shows it.
   Nothing outside the lab moves.
2. **The layer proven on what exists:** J-02 changes no picture (I-01's and P-00's way), so a difference after it is content, never
   plumbing.
3. **The loop early:** J-03 sends a culture and shows the first change, so the game can be played from the third milestone on.
4. **The plain changes before the strange:** air, water, ground and plants (J-04, J-05) use paths that exist (tints, covers, the
   flora's flags).
5. **The animals before their fates:** remains are a world's own animals and changes act on their bodies, so the bodies (J-06, plates
   judged first) and their ways (J-07) come before J-08.
6. **The overgrowth last of the looks:** it covers everything that exists by then, remains and ruins included.
7. **Could go otherwise:** the animals first (J-06 to J-08 before J-01), one word from the user.

Decided by default (2026-11-20), until the user's calls on the sketch:

1. **One release:** all twelve are 1.5.0 (the user: "we are including it in 1.5.0 nonetheless").
2. **Sending:** the drone bay's second trip (S). One drone takes a culture from a flask that shows FULL to a place chosen on the
   world's map, lets it go, watches two minutes and climbs. The flask keeps its culture.
3. **Worlds:** any landable world but the home world, where every game starts. A furnace kills every culture. A people's world can be
   seeded, ruins and all.
4. **Whether it takes:** the solvent, the place's medium (warmth, salt, light, food), oxygen, pH and dose, land (DRY), the natives
   (with the price paid). A line can change in the wild, found when a sample of it is read. The same seeding always gives the same world.
5. **The clock:** the guide's own, seconds the game simulated across saves and expeditions. The warp does not hurry it.
6. **Pace:** the place in about 5 minutes, the region in about 30, the whole world in about 2 hours, read in 24 steps.
7. **Several lines:** up to three a world, each acting on the world the ones before left. Nothing undoes one.
8. **Harm is common:** of seedings that take, about half harm the world's life, a third change only how it looks, a fifth make it more
   livable. `seed` prints the shares over many worlds and lines.
9. **Fourteen world genes** on every species (most doing nothing in the wild), at places 19 to 32. Places 1 to 18 are drawn as now.
10. **Nothing moves the ground:** heights, rivers and lakes stay the world as generated. The made climate moves ice, snow and plants. A
    runaway's seas dry to their old beds (the dead deserts' rule).
11. **Banks:** a made colour first takes the bank of what it replaces, then a free one. At most four made colours a world.
12. **Animals are named by the explorer,** as microbes are. The generated names go.
13. **Loans** carry no seeded world. **Auto-explore** never seeds.

### J-01: world genes

* **Fourteen genes** (`life/genes`): GAS, DYE, LIFT, FROST, SLIME, SHELL, STOP, BIND, ROT, POISON, CARRIER, HOST, DRY, SHIELD. They are
  appended to `GeneKind` and take places 19 to 32 of each genesis's map, on a salt of their own; places 1 to 18 are drawn as now. Their
  tables come after `shape`, each on its own salt. A table gene has 2 letters of choice and 4 of weight, a weight gene 4.
* **Wild values:** every species carries all fourteen. The wild letters give its own values: a methane-maker's GAS, a magnet-turner's
  RUST, the red snow alga's DYE and FROST, a colony's BIND, a dead-eater's ROT, a hunter's POISON; the rest NONE or 0.
* **The lab:** each flask shows them on the incubator's screen and in the room: bubbles, coloured water, scum, frost, sediment, a flask
  overgrown with tissue. POISON and CARRIER show in a mixed flask, SHIELD under a new ULTRAVIOLET light. They are learned as Y-02's genes
  are, against the parent. ROT, HOST and DRY show only on a world (from J-03).
* **The editor:** the map on two pages (CELL GENES 1 to 18, WORLD GENES 19 to 32; Page Down and Page Up turn, as Tab chooses a culture).
  Edits, moves, copies and switches work as
  today. A working GAS, DYE, SLIME or SHELL is priced by its weight.
* Done when:
  * `lab genes` prints the new genes for worlds of four, six and eight letters, forty species' wild values, and each new gene's edit
    before and after;
  * unit checks pass: places 1 to 18, the old tables and every saved edit read as before (on a copy of the player's guide); every
    unedited species of forty worlds is itself; each new gene's edit moves its value within limits; each gene the lab shows is shown and
    learned from one change; POISON and CARRIER in a mixed flask; editor, lines and flasks over a save and load;
  * frames: a flask for each new look (`lab cultures`) and the editor's second page; the flow writes a line with a world gene changed;
  * `tools/prove compare r447`, every difference named; `bench check`; `make check`; `tools/prove snapshot j01`.

### J-02: the world now (nothing drawn changes)

* **The layer:** what a world was made into at a step, as data (`galaxy/made.h`), computed in `life/seeding` from the guide's records.
  The game puts it into `BodyGen` by value where the drawing gets its generator (`SpaceRenderer::genFor`, `SurfaceSite::initFrame`, the
  landing map's probe site), and `unseasoned` and `thenWorld` strip it.
* **Reads,** each doing nothing without a layer: the planet function's last pass (after `darkGround`), the ground's palette, weather
  and climate (`computeEnvironment`), the clouds, the globe's ramps and a land tint, the landing and sector maps (`mapColor`), the data
  sheet. The kind-level reads a made world changes (`tempOffset`, `site.atmosphere`) go through `BodyGen`.
* **Caches** keyed by the step as well as the seed: generators, maps, the telescope's plates, the zoom, the drone map.
* **The record:** the guide's `seeded` and `clock` lines, byte-stable. A loan carries no `seeded` line.
* **A test layer:** `seed test` puts a tint and a cover on a world, to prove every path draws them, and takes them off.
* Done when:
  * the battery against j01 is the same in every item (no layer, no difference);
  * `seed test` draws a world's globe, landing map and ground with the test layer, and without it as before;
  * unit checks pass: the layer the same on any thread and in any order; a map made with it differs, and one without it is the cached
    one; the guide byte-stable with `seeded` lines; the clock the same skipped, stepped and reloaded; a loan without `seeded` lines;
  * `bench check` (no cost on an unseeded world); `make check`; `tools/prove snapshot j02`.

### J-03: sending a culture down

* **The trip:** S on the drone bay, a flask that shows FULL, a place on the world's map, Enter. One drone goes down for two minutes and
  comes back with its report; the log says SEEDED. Refused at home, at a giant, out of orbit, with the drones away or charging, and for a
  fourth line.
* **Whether it takes** (`life/seeding`, a pure function of world, line and place): the solvent, `growthIn` over `mediumOfPlace`,
  oxygen, pH and dose (SHIELD), land (DRY), the natives (`pairOf` with the price). Then the drift in the wild.
* **The spread** on the guide's clock in 24 steps, through the warmth, waters and land the line can live in; faster with LIFT.
* **The first change:** the waters' colours from DYE (RED SEAS and the other colours) and GLOWING SEAS, on the ground, on the maps and
  from orbit.
* **Life:** samples of a seeded world find the line where it spread (drifted or not) and fewer of the natives it beat. The data sheet
  gets SEEDED and MADE lines, the LANDING log what changed.
* Done when:
  * `seed` prints, for forty worlds and six lines (wild, warm, cold, a dye, a methane-maker, STOP cut), the verdict and why, the
    spread's steps and what each shows; frames of a seeded sea from orbit and on the ground, before and after;
  * unit checks pass: the verdict agrees with a flask in the place's medium; the spread the same skipped, stepped and reloaded; the
    drift fixed by world, line and place; the record byte-stable; a loan leaves the borrower's world unchanged; an unseeded world bit
    for bit; the home world refused;
  * the flow seeds a world in its last stage (KI-450: where no later frame moves); a tour stop at the seed page;
  * the battery against j02, every difference named; `bench check`; `make check`; `tools/prove snapshot j03`.

### J-04: air and sky

* **The made climate:** a temperature and a pressure of now (as `nowTempC` is) that move the ice caps, the snow line and the plants'
  health, never the rivers; the HUD's TEMP and ATM, the data sheet's AIR line.
* **Air:** THICK AIR, RUNAWAY (seas steaming, then dry to their old beds), ORANGE HAZE, SULPHUR AIR, OZONE, THIN OZONE, COLD HAZE.
* **Clouds and rain:** GLOWING CLOUDS, COLOURED CLOUDS, BLOOD RAIN (coloured rain and puddles), SEA SMOKE (fog lying on the seas),
  ENDLESS RAIN, SPORE CLOUDS.
* Done when: `seed traits` draws each trait from orbit and on the ground (`shots/tests/made_*.png`); the climate before and after for
  twenty worlds and each gas; unit; the battery against j03; `bench check`; `make check`; `tools/prove snapshot j04`.

### J-05: water, ground and plants

* **Water:** PURPLE SEAS, BLACK SEAS, MILK SEAS, FOAM SEAS (foam banks on the shores), JELLY SEAS (slow heavy waves), SOUP SEAS, SOUR
  SEAS (reefs bleached), RUST RIVERS, BOILING SEAS.
* **Ground:** COLOURED SNOW, DARK ICE (melting), CRUSTS, MATS, STROMATOLITES, RUST GROUND, CHALK GROUND, GLASS SANDS, SULPHUR CRUSTS,
  PITTED STONE, SLIME.
* **Plants:** BLIGHT, ROT (fallen forests), FROST BURN, GALLS (swellings on trunks and limbs), NEW LEAVES, GLOWING FORESTS, LUSH,
  BLEACHED. The globe and the maps follow.
* Done when: frames of each trait; unit; the battery against j04; `bench check` (the forest at 4x within its 13 ms, galls and all);
  `make check`; `tools/prove snapshot j05`.

### J-06: animal bodies (generation 18)

* **A species as a body of parts:** segments with a girth, 0 to 6 pairs of limbs with joints and feet, neck, head (snout, beak, trunk
  or mandibles; 0 to 8 eyes; horns, frills), tail, back (plates, spines, sail, shell), wings, hide and pattern, size. Sixteen plans as
  recipes (the design's section 9). A world's animals share a style, and no two of a world look alike (a measure, as `MORPH_ALIKE`).
* **Drawn:** a mesh built once a species and posed each frame, on the pool. Hides in two banks of their own a world; flyers with
  bodies; several swimmer species; far animals in lines, as now.
* **Generation 18:** each species from its own seed; a row and a section in `generations.md`; the regress blessed after the bump. The
  player's guide loads with the regeneration notice.
* **Plates to judge:** `bestiary plates` draws sixty-four worlds' animals side by side and in herds, for the user's verdict before J-07.
* Done when:
  * the plates; unit checks pass: species pure functions, under 1.5% of 1,000 species alike, every plan's poses within bounds, the
    herds' cells as before;
  * the scenes `creatures_lineup` and `felisian_herd` made again (named); `regress` blessed for the bump, then 24 of 24 both ways;
  * the battery against j05, every difference named; `bench check` (the herd within 0.6 ms at 1x, the surface's budgets); `make check`;
    `tools/prove snapshot j06`.

### J-07: animal ways and the record

* **Gaits** for every body: waves of legs, trot, gallop, bound, hop, slither, crawl, flap, glide, swim, drift.
* **Ways:** grazers, browsers reaching up, hunters stalking and chasing, scavengers, burrowers, climbers, flocks and swarms, the
  solitary and the territorial, night walkers; swimmers in shallows and breaking the surface; floaters on the wind.
* **The record:** each species seen kept in the guide, N names one close by, the data sheet's LIFE line by what was seen. Loans carry
  the names as microbes' names travel.
* Done when: `bestiary ways` prints and draws each way; a scene each for a hunt, a flock and a swimmer; unit (gaits within bounds, the
  record byte-stable, a loan's names); the battery against j06; `bench check`; `make check`; `tools/prove snapshot j07`.

### J-08: the animals' fates

* **Die-offs:** a share of each species dies of its cause. Remains are the species' own bodies (fresh, bloated, sunken, bones,
  scattered), thickest at water; fliers fallen, swimmers washed up, flies, hunters feeding. A few old bones lie on wild worlds too.
* **Sickness:** WASTING, MELTING, GROWTHS, SPROUTING (the animal climbs to high ground and dies there), BLOATED, CRAZED, STONE SKIN.
* **Changed young:** the list in the design's section 8, one to three changes an animal, chosen by the line's genes and a hash of the
  species; a share of each herd.
* **Thriving** and **silence.** The record marks species DYING, GONE or CHANGED.
* Done when: `seed fates` draws each fate and each change; a seeded world's herds over its steps; unit (the shares fixed by world and
  line, remains a pure function of the cell, the marks); the battery against j07; `bench check` (a herd with remains within its
  budget); `make check`; `tools/prove snapshot j08`.

### J-09: the overgrowth

* **Types** by how the line lives and its genes: FLESH, FUNGUS, SLIME MOULD, LICHEN, CORAL, SPONGE, HAIR, BLADDERS, CARPETS, FILMS.
  Variants by colour, texture, grain, wetness, hardness, light and hair.
* **The cover:** three new materials (soft tissue, threads, hard), a detail tile for each texture, a ragged creeping edge. Trees and
  rocks are wrapped, remains taken in. From orbit a spreading stain, with lit veins at night.
* **Structures:** mounds, ridges, blisters and sacs, stalks, folds, breathing vents, rib arches and teeth, membranes, polyps, and each
  type's own (caps and shelves, creeping veins, spires, vases, waving hair, rising bladders); drawn and walked round.
* **Plates first:** `overgrowth plates` draws each type and forty variants on a test ground, for the user's verdict.
* Done when: the plates; frames of an overgrown world from orbit, on the landing map and on the ground at its steps; unit (the cover a
  pure function of place and step, the structures' colliders, no tree in a structure); the battery against j08; `bench check` (the
  surface and the forest on an overgrown world within their budgets); `make check`; `tools/prove snapshot j09`.

### J-10: ruins overgrown, and the sound

* **Ruins:** walls taken from the foot up (the rasteriser's second bank, the edge climbing with the steps); doorways and windows grown
  shut with membranes the explorer pushes through; nodules and drapes; floors covered; graves and works too. PITTED STONE crumbles them.
* **Sound:** WET GROUND (steps squelch), BREATHING (vents), creaks, hair hissing in the wind; the silence of a dead world.
* Done when: frames of a city and a capital overgrown at their steps; WAVs of the sounds within the unit's bounds; unit (colliders,
  membranes passable); the battery against j09; `bench check` (city and capital at 4x within 8.3 ms); `make check`;
  `tools/prove snapshot j10`.

### J-11: made kinds

* **A world past a line** takes a made kind on the data sheet, with its heights and rivers as generated: MADE HOTHOUSE (the scorched
  felisian's cover), MADE SNOWBALL (the frozen felisian's), HAZE WORLD, PURPLE WORLD, RUST WORLD, the overgrown worlds by type (FLESH
  WORLD and the rest), CHARNEL WORLD, GREENED WORLD. The kind line ("FLESH WORLD, ONCE FELISIAN"), the sentence and the globe follow.
* Done when: frames of each made kind; `seed` prints which of forty worlds and six lines end as which kind, and the shares of harm,
  looks and livable; unit; the battery against j10; `bench check`; `make check`; `tools/prove snapshot j11`.

### J-12: the record and the close

* **The catalogue:** native species DIED OUT (with the line that did it), the line found wild and drifted, animals GONE or CHANGED.
* **The LIFE page:** worlds seeded, made kinds, species died out. Auto-explore lands on seeded worlds as on any.
* **The docs:** a reference note for seeded worlds, `rules-life-seeding.md`, the rules it changes ("the worlds stay as generated" to
  `superseded.md`), KNOWN-ISSUES rows, the generated pages.
* Done when: from a clean build, the whole battery against j11; `bench check`; `make check`; `tools/prove snapshot j12`.
