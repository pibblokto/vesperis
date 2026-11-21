# Status

Where the work stands. Each milestone, bug or request batch rewrites this file (`docs/CLAUDE.md`). What each item did is in its
log entry (`docs/decisions/log/`), B file or R file.

## Releases

- **Released:** 1.4.0 on 2026-10-29: branch `release/v1.4.0` merged into main by pull request #9, tagged `v1.4.0`, six zips
  (`packaging/RELEASE.md`).
- **In progress:** 1.5.0, "the small and the spacious", on branch `release/v1.5.0` (made from main bb0e58b on 2026-11-16). The plan is
  `docs/ideas/ITERATION-1.5.0.md`, the design `docs/ideas/IDEAS-small-and-spacious.md`, `docs/ideas/IDEAS-genes.md` and
  `docs/ideas/IDEAS-seeded-worlds.md`.

## Done in 1.5.0

Phases 1 to 5 are done; Phase 6, the J series, has J-01 to J-06 done (below). Log entries are in `docs/decisions/log/7-1.5.0.md`.

- **Microbes (Z-01 to Z-07, R-425):** life on every landable world (`src/life/`, `docs/reference/13-life.md`), with `GEN_VERSION` 17;
  each species' form and its slide in six ways of seeing; the search on the ground (O takes a sample); the microscope, the incubator,
  the sequencer and the tree of kin; the catalogue, the LIFE statistics page and the data sheet's microbe line.
- **The ship (I-01 to I-04, R-426):** the new Stardrifter with three decks and eight rooms, its light and materials, and life aboard.
- **Genes (Y-01 to Y-04, R-443, R-444):** trees of kin by world and by folder; the gene editor (cut, change, move genes in from kin,
  copy, switch) and strains written from it into flasks.
- **Player requests:** R-427 to R-442 and R-445 to R-447 (the ship's rooms, the lab's lists, folders, experiments, naming, the write
  prompt, plain English). Each has an R file in `docs/iteration-3/plans/requests/`.
- **Bugs fixed:** B-434 to B-446, each with a B file in `docs/iteration-3/bugs/resolved/`.
- **R-447, jars taken out** on the user's word: the incubator has one page, the flasks; the shelf over the bow bench is gone.
- **J-01, world genes** (2026-11-20): every species carries fourteen new genes at places 19 to 32 (gas, dye, lift, frost, slime,
  shell, stop, bind, rot, poison, carrier, host, dry, shield), read from what it is. In a flask they show as bubbles, dyed water, a scum,
  slime, a sediment, clumps, tissue, ice, a poisoned neighbour or genes passed on; the incubator has a UV light. The gene editor's map
  has two pages (Page Down and Page Up).
- **J-02, the world now** (2026-11-20): a seeded world's layer (`galaxy/made.h`, `life/seeding`) rides in the world's generator and is
  read last by the planet function, the globe, the telescope's plate, the maps and the ground; the guide's `seeded` and `clock` lines.
- **J-03, sending a culture down** (2026-11-21). S on the drone bay opens the seed page: a flask that shows FULL, a place on the world's
  map, Enter. Drone 1 lets the culture go and reports whether it took, and why not. A line that takes spreads through the waters, land
  and warmth it can live in; a dye turns those waters its colour (RED SEAS and the rest), and a glowing line lights them at night.
  Vials find the line where it spread; the data sheet has SEEDED and MADE lines.
- **J-04, air and sky** (2026-11-21). A line that gives off a gas changes its world's air over the two hours it spreads: CO2 and methane
  warm it (THICK AIR), methane hazes it orange where no oxygen burns it, oxygen forms an ozone, hydrogen sulphide yellows the horizon,
  dimethyl sulphide cools it under a pale haze and lays a fog on its seas, and a living world with seas past 42 C runs away (its seas
  steam, then dry to their beds). A line with LIFT carries its glow, dye, ice or spores up: glowing clouds at night, red clouds and
  blood rain with puddles, endless rain, spore clouds. On a living world the ice caps, the snow line and the plants follow the warmth;
  the HUD's TEMP and ATM and the data sheet's AIR line read it.
- **J-05, water, ground and plants** (2026-11-21, R-448's fifth milestone). What a line makes of the waters, land, snow and
  plants it lives in, each with a colour of its own: soup, jelly, milk and rust waters, foam on the shores; mats and stromatolites on
  its shores, crusts, chalk, glass sands, sulphur crusts, rust, slime and pitted stone on its land; coloured snow and dark ice; a host
  of plants' glowing forests, new leaves, frost burn and galls; a wood rotter's blight and fallen forests. From the air: purple or
  black seas, sour seas with bleached reefs, rusted ground, lush or bleached plants, boiling seas. The landing map, the globe and the
  ground show them. A world nobody seeded is drawn as before, bit for bit.
- **Latest (2026-11-21): J-06, animal bodies** (generation 18). Every world's animals are drawn again: each species from its own seed,
  a body of parts in sixteen plans (quadrupeds, long-necked browsers, hexapods, octopods, striders, hoppers, giants, serpents,
  many-legged crawlers, slugs, burrowers, climbers, gliders, flyers, floaters and swimmers shaped as fish, eels, rays or long necks). A
  world's animals share a style and two hide colours of their own, and no two of a world look alike. Flyers have bodies near, several
  swimmers break the surface, floaters drift in a thick air. Saves and guides of generation 17 load with WORLDS REGENERATED (GEN 17, NOW
  18). The plates for your verdict: `shots/tests/plates/` (`bestiary plates`).

## Planned (2026-11-20)

- **R-448, seeded worlds:** the J series, J-07 to J-12 left (`ITERATION-1.5.0.md`, Phase 6; `IDEAS-seeded-worlds.md`). A gene-edited
  culture sent down by a drone changes a world, more often for the worse: made traits, die-offs with remains, changed animals, worlds
  overgrown with flesh, fungus or other tissue, ruins included; the animals' ways and their record (J-07).

## Committed

Commits on `release/v1.5.0`, each on the user's word, none pushed: 0c90433 (Z, I, B-434 to B-438), 455c7ad (B-439), 82592c5 (B-440,
R-427 to R-434), d12a9ea (R-435 to R-442, B-441, B-442), 20b236e (B-443, Y-01, Y-02), c9e98be (Y-03, Y-04, R-445, R-446, B-444 to
B-446), fce6383 (R-447, the R-448 plan, J-01 to J-05). Not committed: J-06.

## Next

- **Your verdict on J-06's plates** (`shots/tests/plates/`), then **J-07, animal ways and the record** (gaits for every body, grazers,
  hunters, flocks, swimmers in the shallows; each species seen kept in the guide and named by the explorer) on the user's "proceed".
- The 1.5.0 release after J-12, on the user's word (the `release` skill: pull request, tag `v1.5.0`, six zips; the Linux builds need
  Docker, asked first).
- **Proof base:** `build/prove/j06` (40 items: J-06 added `bestiary`).

## Playing the lab

In the laboratory, E opens a device. Microscope: vials by world and cultures by folder; on a slide L light, M magnet, H warmth, F food,
K keeps a cell. Incubator: flasks (a digit types the warmth, N names, K keeps a copy). Sequencer: reads a genome. Tree of kin: the
screen on the port wall. Gene editor: K puts a kin beside it, M moves its gene in, D copies, S switches, Enter edits letters, C cuts, W
writes a strain (folder, then name), Page Down the world genes. The incubator's light field has UV. `lab microscope`, `lab cultures`,
`lab tree` and `lab genes` draw them into `shots/tests/`.

## Seeding a world

Grow a culture in a flask until it shows FULL, park in orbit of a landable world (not home), E at the drone bay in the hangar, then S:
C picks the flask, the arrows move the place (Shift for ten degrees), Enter sends a drone. Its report comes about two and a half minutes
later. A dye line turns the waters it lives in its colour within minutes near the place and over the whole world in one to four hours;
land to see it, look from orbit or open the landing map. A line that gives off a gas (set its GAS gene on the editor's second page)
changes the air from about half an hour in to two hours: land to read TEMP and ATM, or open the data sheet's AIR line. A line with LIFT
carries its dye, glow, frost or spores into the clouds. A line's other world genes act where it lives: SHELL (chalk, glass, rust,
sulphur), SLIME, BIND, STOP cut, DRY, ROT (wood, stone), and HOST set to plants with its dye, glow, frost or CARRIER. The data sheet's
second page has SEEDED, MADE and AIR. `seed worlds`, `seed sea`, `seed climate`, `seed traits` and `seed looks` show the verdicts, the
seas, the climates, every made air and every made water, ground and plant in `shots/tests/`.

## Seeing the animals

Land on a world with plants: herds graze in the open, flyers circle overhead and perch in trees at dusk, swimmers break the surface of
seas and deep lakes, and floaters drift in a thick air. X marks the creatures nearby. `bestiary plates` draws 64 worlds' animals one by
one and a herd each into `shots/tests/plates/` (`plate_NN.png`, `herd_NN.png`, all of them small in `plates.png`).

## Playing the ship

Shift runs, Page Up takes the lift, U dims the lights, Y makes the hull glass. E works the bunk, the desk, the music player's deck, the
capsule, the quarters' wall of photographs and a garden bed. The buggy and the drone stand in the maintenance room astern of the hangar.
On the ground O takes a sample, Shift+O shows the activity vision, Shift+U the ultraviolet lamp. `ship rooms`, `ship light`, `ship life`
and `life kit` draw them.

## Open with the user

- **J-06's plates:** your verdict on the bodies, the plans, the colours and the herds before J-07 (the plan's "plates judged first").
- **R-448's calls:** the plan's defaults hold (the drones send a culture, about two hours a world, the animals in the middle, home
  never seeded, the explorer names the animals) until the user says otherwise.
- **Docs still in the old style:** the reference notes (`docs/reference/`, 1.4 MB) and the code comments, until each is next edited.
- Standing leave to commit each step; the clangd-lsp plugin; B-440's leftovers in their `shots/` (`probe_1`, `probe_2`).
- Known issues (`docs/iteration-3/bugs/KNOWN-ISSUES.md`): KI-424, KI-425, KI-428 to KI-431, KI-435 to KI-438, KI-440, KI-444,
  KI-446, KI-448 to KI-450, KI-453 (the data sheet calls a second sun's planet a moon), KI-454 (a heard sound's status line can run
  off the screen), KI-455 (twelve different world looks dim the last flasks on the shelves), J-04's KI-456 to KI-459 (only a living
  world's ice, snow and plants move, and only it runs away; samples read the old warmth; a dyed rain without a bank), J-05's KI-460
  to KI-464 (one colour a class; the globe shows one class; a line's air ignores its plants; a crust's forms in one texture; four lent
  banks) and J-06's KI-465 and KI-466 (one hide bank on a crowded world; the LIFE line cut).
- Offered by B-430: the tables' dump as a harness mode for the container; parallel.h's misleading indentation (38 of the Linux builds'
  46 warnings). Offered by B-431: the `returnState =` lines before a scene's `openScreen`, now redundant.
- Our Docker images `vesperis-build:arm64` and `:amd64` were removed on the user's word. The old proof kits (`build/p00` to
  `build/d05`, about 9 GB) are deleted only on the user's word.

## Disk

About 89 GB free (2026-11-21). `build/prove` keeps the snapshots r434, r438, r442, b443, y01, y02, y03, y04, r446, r447, j01, j02,
j03, j04, j05 and j06, the compares' outputs against r434 to j05, and their logs: about 41 GB. A snapshot and a compare's outputs are
about 1.4 GB each.

## Next free ids

Bugs B-447, requests R-449, known issues KI-467. Milestone ids go by letter series in `docs/ideas/MILESTONES.md` (1.5.0 took Z, I,
Y and J).
