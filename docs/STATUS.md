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

Phases 1 to 6 are done: the J series closed with J-12 (below). Log entries are in `docs/decisions/log/7-1.5.0.md`.

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
- **J-06, animal bodies** (2026-11-21, generation 18; the user on the plates: "Plates look great"). Every world's animals are drawn
  again: each species from its own seed, a body of parts in sixteen plans (quadrupeds, long-necked browsers, hexapods, octopods,
  striders, hoppers, giants, serpents, many-legged crawlers, slugs, burrowers, climbers, gliders, flyers, floaters and swimmers shaped as
  fish, eels, rays or long necks). A world's animals share a style and two hide colours of their own, and no two of a world look alike.
  Flyers have bodies near, several swimmers break the surface, floaters drift in a thick air. Saves and guides of generation 17 load
  with WORLDS REGENERATED (GEN 17, NOW 18). The plates: `shots/tests/plates/` (`bestiary plates`).

- **J-07, animal ways and the record** (2026-11-21). Each species has a way of living (it grazes, browses, hunts, scavenges, burrows or
  climbs; flies in a flock, a swarm or high as a soarer; swims in a shoal, the shallows or leaps; a floater drifts) and lives in herds,
  alone or on a ground of its own. Gaits follow speed and the feet stay planted. The generated names are gone: the guide keeps each
  species seen, an unnamed one goes by its words ("STRIPED QUADRUPED"), N names the animal under the crosshair. `bestiary ways`.

- **J-08, the animals' fates** (2026-11-21). A seeded line's genes and the air it makes now reach a world's animals: die-offs
  with remains (the species' own bodies, fresh, bloated, sunken, bones, scattered; thickest by the water, flyers fallen, swimmers washed
  up, a dried-blood stain and flies over the fresh), sicknesses (wasting, melting, growths, sprouting, bloated, stone skin, crazed, a glow
  or a dye in the skin), changed young (42 changes, one to three a species, steered by the line's genes), thriving and silence. Hunts can
  end in a kill, and hunters and scavengers feed at remains (KI-468). The record marks species DYING, GONE or CHANGED. A world nobody
  seeded is as before but for a few old bones. `seed fates` draws each fate into `shots/tests/fates/`.

- **J-09, the overgrowth** (2026-11-21). A seeded line that never stops (its STOP cut) and binds its cells now grows over the
  land it lives on, from its eighth step to the whole world by its 32nd: flesh, fungus, slime mould, lichen, coral, sponge, hair,
  bladders or carpets by its genes and way of living, and films by the water where it slimes without binding. Its colour, texture, grain,
  wetness, hardness, glow and cilia come from its genes. Three new ground covers (TISSUE, THREADS, HARD GROWTH); eighteen kinds of
  structure on them (mounds, ribs, teeth, membranes, caps, shelves, veins, spires, vases, hair, floating bladders and more), walked round;
  forests wrapped in it, rocks too, remains taken in; a dark stain spreading from orbit, lit veins at night where it glows. A world nobody
  seeded is drawn as before. `overgrowth plates` and `overgrowth world` draw it into `shots/tests/overgrowth/`.

- **J-10, ruins overgrown, and the sound** (2026-11-22). An overgrowth now takes the ruins it reaches: its cover climbs each
  building from the foot, faster as the steps go on (a house's walls in about six steps after its edge passed, a tower's in twelve, a
  capital's tallest in about twenty-five), with a ragged edge up the walls; windows go under it, membranes grow across the doorways and
  windows it climbed past (the explorer pushes through and they tear), nodules stud the walls round its edge, drapes hang from the tops
  it nearly reached, and the streets under it stay covered. A stone rotter's line (PITTED STONE) crumbles the settlements it reached.
  Steps squelch on a wet cover (WET GROUND), a flesh's or a sponge's vents breathe (BREATHING), the tall and hard creak in the wind, hair
  hisses; on a world gone silent nothing living is heard. `overgrowth ruins` draws a city and a capital at four steps and a city crumbled.

- **J-11, made kinds** (2026-11-22). A world whose change passed a line takes a made kind on the data sheet ("FLESH WORLD, ONCE
  FELISIAN"), with its own sentence: MADE HOTHOUSE (its seas boiled away, the scorched felisian's cover over its own ground, the forests
  stone), MADE SNOWBALL (a living wet world cooled under -6 C ices to its equator in six steps, FREEZING OVER, then lies under the frozen
  felisian's cover), HAZE WORLD, PURPLE WORLD, RUST WORLD, an overgrown world by its type (FLESH WORLD, CORAL WORLD and the rest),
  CHARNEL WORLD (every animal and plant dead) and GREENED WORLD (a cold or dry world made warmer or wetter where nothing was harmed: its
  plants lush, its animals thriving). Heights and rivers stay as generated. A made hothouse no longer rains, and a warming's heat now
  harms by how warm the world gets (a tundra warmed to 6 C greens). `seed kinds` prints which kind forty worlds and six lines end as, with
  the shares of harm, looks and more livable, and draws each kind from orbit, on the data sheet and on the ground.

- **Latest (2026-11-22): J-12, the record and the close.** Once a line has covered its world, the natives it beat there that live
  nowhere it does not die out: no vial of the world holds them again. In the catalogue (G, THE CATALOGUE) a seeded world's head names
  its made kind, a native seen that died out reads DIED OUT (its sheet says which line and how: POISONED BY NERVE BLOOM), a line seen
  under the microscope in a vial of its world is found wild (SEEDED; its sheet says what the wild changed once the sequencer read it
  there), and every world's animals seen follow its species with what the ground showed (GONE, DYING, CHANGED), each with a sheet in
  words. The LIFE page adds WORLDS SEEDED, MADE and DIED OUT. Auto-explore lands on seeded worlds as on any. The seeded worlds have a
  reference note of their own (`docs/reference/14-seeded-worlds.md`) and `rules-life-seeding.md`. `seed record` draws a poisoned
  world's record into `shots/tests/record/`.

## Planned (2026-11-20)

- Nothing left of 1.5.0's plan: R-448's J series is done. The release follows on the user's word.

## Committed

Commits on `release/v1.5.0`, each on the user's word, none pushed: 0c90433 (Z, I, B-434 to B-438), 455c7ad (B-439), 82592c5 (B-440,
R-427 to R-434), d12a9ea (R-435 to R-442, B-441, B-442), 20b236e (B-443, Y-01, Y-02), c9e98be (Y-03, Y-04, R-445, R-446, B-444 to
B-446), fce6383 (R-447, the R-448 plan, J-01 to J-05), 1b5ea42 (J-06), 47d4d2d (J-07), 599ff5f (J-08 to J-11). Not committed: J-12.

## Next

- **The 1.5.0 release** on the user's word (the `release` skill: pull request, tag `v1.5.0`, six zips; the Linux builds need Docker,
  asked first). First, the user's verdict on J-12's frames (`shots/tests/record/<catalogue|died_out|found_wild|animal_gone|life_page>.png`),
  J-11's (`shots/tests/kinds/`) and J-10's (`shots/tests/overgrowth/ruins_*.png`, the sounds `shots/tests/sound_squelch.wav`,
  `vents_dry`, `vents_wet`, `creaks`, `hair`, `tear`), and a commit of J-12.
- **Proof base:** `build/prove/j12` (44 items; J-12 added `seed_record`).

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
seas, the climates, every made air and every made water, ground and plant in `shots/tests/`. J-09: a line with its STOP cut and its
BIND set overgrows the land it lives on from its eighth step on (a SHELL of chalk makes coral, of glass a sponge; LIFT bladders; SLIME
without BIND films by the water): land by the place to walk among it, or watch its stain spread from orbit. J-10: let it go by a town of a
people's world and land there later: the cover climbs the walls, the doorways grow shut (walk through them), and a wet one squelches
underfoot; a line with ROT set to stone crumbles the ruins it reaches. J-11: once a world's change passes a line, the data sheet names
its made kind; a CO2 line on a warm wet world runs away and dries its seas (MADE HOTHOUSE), a dimethyl sulphide line on a tundra freezes
it (MADE SNOWBALL), a CO2 line on a tundra greens it. `seed kinds` draws each kind into `shots/tests/kinds/`. J-12: look at a world's
species under the microscope before seeding it with a POISON line, and after about two hours (the line over the whole world) open the
catalogue: those it drove out read DIED OUT. Sample the world again and put the line's vial under the microscope to find it wild, then
read it on the sequencer to see what the wild changed in it. The statistics' L page counts the worlds seeded and what died out.

## Seeing the animals

Land on a world with plants: herds graze in the open, hunters stalk them, flyers circle overhead and perch in trees at dusk, swimmers
break the surface of seas and deep lakes, and floaters drift on the wind. X marks the creatures nearby. Put the crosshair on an animal
within 40 m (its words show over the stamina, "A STRIPED HOPPER - N") and N names it; the data sheet's LIFE line lists what you saw.
`bestiary ways` draws each way, and every land body's walk and run, into `shots/tests/ways/`. J-08: seed a world with a line that harms
(a working POISON, a HOST of animals, a sulphide GAS: the editor's second page), wait an hour or so and land by the place: remains lie
where the herds lived and by the water, flies over the fresh, the sick and the changed among the living; X and the data sheet's LIFE
line say DYING, GONE or CHANGED. `seed fates` draws each fate, each change and a herd over its steps into `shots/tests/fates/`.
`bestiary plates` draws 64 worlds' animals one by one and a herd each into `shots/tests/plates/` (`plate_NN.png`, `herd_NN.png`, all
of them small in `plates.png`).

## Playing the ship

Shift runs, Page Up takes the lift, U dims the lights, Y makes the hull glass. E works the bunk, the desk, the music player's deck, the
capsule, the quarters' wall of photographs and a garden bed. The buggy and the drone stand in the maintenance room astern of the hangar.
On the ground O takes a sample, Shift+O shows the activity vision, Shift+U the ultraviolet lamp. `ship rooms`, `ship light`, `ship life`
and `life kit` draw them.

## Open with the user

- **J-07's calls:** N names the animal the crosshair rests on and opens the map otherwise (Shift+N would keep N the map's alone);
  swimmers of the shallows are rare (KI-467).
- **J-08's calls:** old bones lie by a herd's cell in eight on every world, seeded or not (the design's "so remains alone don't always
  mean a seeding"); a hunt kills by a chance of 0.15 to 0.6; only land herds show a sickness or changed young (KI-470).
- **J-09's calls:** the user said "Proceed" after the report without a word on the plates; one overgrowth colour a world (KI-473);
  the forest keeps its trees under the cover, wrapped, while grass and undergrowth go; the stain takes the globe's made bank before a
  seeded sea's tint (bank 31 then, the first disc only: KI-475); the open ground's overgrown bench rows have budgets of their own (7.5
  and 10.3 ms), the landing's surface rows looking at the sea.
- **J-10's calls:** the climb's pace (above); a building taken as one, a taken one drawn five shades under the ground (KI-477); only
  the walls' and rings' openings grow shut (KI-478); a torn membrane heard, not seen (KI-479); a stone rotter eats mud brick too; J-09's
  walk-into check had run with the controls off (the explorer stood still): it walks now, and still none passes.
- **J-11's calls:** the order and the lines of the kinds (above; `rules-life-kinds.md`); `seed kinds`' six lines are a world's own sea
  line with five gases or overgrowing, as J-03's one line took on 21 of 240 seedings; the shares (50% harm, 20% looks, 11% more
  livable, 19% nothing) are printed, not tuned (KI-483); the heat's harm now weighs the mean a world reaches, so a cold world warmed
  greens (it moved J-04's and J-08's rule); a made hothouse no longer rains.
- **J-12's calls:** a native a line beat dies out once the line covers the world, where it lives in no niche or salt the line does not
  (its warmth not read: KI-487), and then leaves every vial of the world; the catalogue lists the animals seen by world, with sheets in
  words (KI-488); DIED OUT counts the explorer's own sheets alone; what the wild changed shows once the sequencer read the line from a
  vial of its world; the seeded worlds' notes moved out of `13-life.md` into `14-seeded-worlds.md`, and sending a culture down out of
  `rules-life-world.md` into `rules-life-seeding.md`.
- **R-448's calls:** the plan's defaults hold (the drones send a culture, about two hours a world, the animals in the middle, home
  never seeded, the explorer names the animals) until the user says otherwise.
- **Docs still in the old style:** the reference notes (`docs/reference/`, 1.4 MB) and the code comments, until each is next edited.
- Standing leave to commit each step; the clangd-lsp plugin; B-440's leftovers in their `shots/` (`probe_1`, `probe_2`).
- Known issues (`docs/iteration-3/bugs/KNOWN-ISSUES.md`): KI-424, KI-425, KI-428 to KI-431, KI-435 to KI-438, KI-440, KI-444,
  KI-446, KI-448 to KI-450, KI-453 (the data sheet calls a second sun's planet a moon), KI-454 (a heard sound's status line can run
  off the screen), KI-455 (twelve different world looks dim the last flasks on the shelves), J-04's KI-456 to KI-459 (only a living
  world's ice, snow and plants move, and only it runs away; samples read the old warmth; a dyed rain without a bank), J-05's KI-460
  to KI-464 (one colour a class; the globe shows one class; a line's air ignores its plants; a crust's forms in one texture; four lent
  banks), J-06's KI-465 (one hide bank on a crowded world) J-07's KI-467 and KI-469 (the shallows' swimmers rare; a climber's side feet
  off a thin trunk), J-08's KI-470 to KI-472 (fates on land herds alone; a cell come back to starts whole; one `seed fates` frame),
  J-09's KI-473 to KI-476 (one overgrowth colour a world; its lent bank; the second disc's tint under a stain; structures on a
  landing's grid), J-10's KI-477 to KI-481 (a building taken as one; arches not grown shut; a torn membrane unseen; a stone rotter
  alike everywhere; a ruin's cover without gloss or night light), J-11's KI-482 to KI-486 (a made kind named on the data sheet and the
  catalogue alone; the shares not tuned; a snowball's sea ice white; a kind with no memory; the cold's harm by its size alone), J-12's
  KI-487 and KI-488 (a native driven out in warmths the line cannot live in too; the catalogue's animals by world alone, in words).
- Offered by B-430: the tables' dump as a harness mode for the container; parallel.h's misleading indentation (38 of the Linux builds'
  46 warnings). Offered by B-431: the `returnState =` lines before a scene's `openScreen`, now redundant.
- Our Docker images `vesperis-build:arm64` and `:amd64` were removed on the user's word. The old proof kits (`build/p00` to
  `build/d05`, about 9 GB) are deleted only on the user's word.

## Disk

About 69 GB free (2026-11-22). `build/prove` keeps the snapshots r434, r438, r442, b443, y01, y02, y03, y04, r446, r447, j01, j02,
j03, j04, j05, j06, j07, j08, j09, j10, j11 and j12, the compares' outputs against r434 to j11, and their logs: about 61 GB. A snapshot and
a compare's outputs are about 1.4 GB each.

## Next free ids

Bugs B-447, requests R-449, known issues KI-489. Milestone ids go by letter series in `docs/ideas/MILESTONES.md` (1.5.0 took Z, I,
Y and J).
