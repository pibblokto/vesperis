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

Phases 1 to 6 are done: the J series closed with J-12, and its known issues were fixed after it (B-447 to B-471, below). Log entries are
in `docs/decisions/log/7-1.5.0.md`.

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

- **J-12, the record and the close** (2026-11-22). Once a line has covered its world, the natives it beat there that live
  nowhere it does not die out: no vial of the world holds them again. In the catalogue (G, THE CATALOGUE) a seeded world's head names
  its made kind, a native seen that died out reads DIED OUT (its sheet says which line and how: POISONED BY NERVE BLOOM), a line seen
  under the microscope in a vial of its world is found wild (SEEDED; its sheet says what the wild changed once the sequencer read it
  there), and every world's animals seen follow its species with what the ground showed (GONE, DYING, CHANGED), each with a sheet in
  words. The LIFE page adds WORLDS SEEDED, MADE and DIED OUT. Auto-explore lands on seeded worlds as on any. The seeded worlds have a
  reference note of their own (`docs/reference/14-seeded-worlds.md`) and `rules-life-seeding.md`. `seed record` draws a poisoned
  world's record into `shots/tests/record/`.

- **The J series' known issues, B-447 to B-471 (2026-11-22)** (KI-455 to KI-486, the user: "Now let's handle all of these").
  A B file each in `docs/iteration-3/bugs/resolved/`. The made climate: every kind's caps and frosts follow a made warmth, a living
  world's ice sheets melt to bare ice; vials, a later line and the spreads' bands read the made warmth; an ocean, a hycean or an eyeball
  runs away to a dry floor of salt; a line's share counts its plants; a made snowball's sea ice is blue and a greened world's biomes move;
  a made kind remembers (a runaway goes on, a snowball thaws only past its ice's own cooling); the cold weighs the mean reached; the shares
  tuned (49% harm, 19% looks alone, 19% more livable; SILVER CLOUDS of methane on an oxygen world). The made colours: one pool of every
  bank a world leaves free, each line its own colour where it lies (the ground, the maps, the globe; where several lie, the one that
  colours), a line's glow in its own plants alone, the globe five or six made banks a disc, a flask past the ship's banks in the nearest
  colour. The overgrowth: a crust in its form, LUSH grows trees, the structures fixed to the world, ruins taken wall by wall, arches and
  gates grown shut, torn membranes kept, a stone rotter where it lives, the ruins following the steps, the cover's gloss and night light.
  The animals: the shallows' swimmers found, a climber round its trunk, flocks and shoals sick and changed, kills kept and remains aged,
  `seed fates`' soup seas. Every screen names a made kind.

- **Play notes (2026-11-22): R-449 to R-453, B-472 and B-473** (the user's list, talked through first, then "Proceed"). An R or
  B file each. R-449: the sequencer and the gene editor take ten seconds; the drones never charge; a flask grows five times as fast (full
  in 2 to 4.5 minutes at its best) and stays full; a seeded world changes four times as fast (a line over its world in half an hour).
  B-472: the editor's and the sequencer's choosers name a kept strain in full. B-473: the seed page offers every culture alive in a flask
  and says its stage. R-450: a gene is known for its species the moment it is changed on the editor; the editor says what its kind is
  for, and with its letters open shows how they set it and what each letter at the cursor would make of it; W refuses a line a vital
  gene's change would kill. R-451: the seed page shows what the culture needs and what the place gives, and a failed seeding's report
  says why in full. R-452: Shift+N names an animal, N is always the map. R-453: GREEN and PINK dyes.

- **Latest: J-15, greening a desert** (2026-11-23, R-454's last milestone). On a desert a light-feeder's carpet (DRY, STOP cut, BIND,
  no SHELL or LIFT) or lichen (the same with a SHIELD) grows grass and shrubs where it lies, from four steps after it covered the ground,
  thick from twelve, in its pigment's colour (a dye's over it): a green pigment greens it. Once they cover half its land the desert is a
  GREENED DESERT on the data sheet ("A CARPET OF CELLS HAS GROWN INTO GRASS AND SHRUBS OVER MOST OF ITS BARE LAND."); its carpet harms
  nothing. On the ground: tufts and darker shrubs, no trees. The gene editor's cut STOP now says what a line overgrows as ("CUT OUT:
  OVERGROWS AS CARPETS, GREENS A DESERT"). `seed kinds`: a green light-feeder tuned for a desert greens it from its 27th step.
- **J-14, the clouds** (2026-11-23): Page Up on a venusian or acid world's seed page climbs into its clouds (70 C down to 0 C); acid life
  lives there, water life never; a cloud line's dye streaks the deck from orbit and its glow lights the night side.
- **J-13, the cold worlds** (2026-11-22): brine lines to -45 C, places at their own warmth, Page Down down a drill hole (the rock's 1 to
  30 km, or a sea under the ice); RED CRACKS; tuned lines on 30 of the 33 test worlds that are not furnaces.

## Planned (2026-11-23)

- **J-16, the known issues** (`docs/ideas/ITERATION-1.5.0.md`, phase 8), on the user's word of 2026-11-23 ("after that there should be a
  J-16 that will basically handle all K-xxx that were created"): every known issue 1.5.0 found that is still open, 36 of them (KI-435 to
  KI-454's twelve, KI-487 to KI-510's twenty-four), each fixed with a B file or closed for a reason the user agrees to. Then the release,
  on the user's word.

## Committed

Commits on `release/v1.5.0`, each on the user's word, none pushed: 0c90433 (Z, I, B-434 to B-438), 455c7ad (B-439), 82592c5 (B-440,
R-427 to R-434), d12a9ea (R-435 to R-442, B-441, B-442), 20b236e (B-443, Y-01, Y-02), c9e98be (Y-03, Y-04, R-445, R-446, B-444 to
B-446), fce6383 (R-447, the R-448 plan, J-01 to J-05), 1b5ea42 (J-06), 47d4d2d (J-07), 599ff5f (J-08 to J-11). Not committed: J-12,
B-447 to B-471 (J-12's tree alone is kept as the git tree object 404ae0c, so the two can be committed apart), R-449 to R-453 with
B-472 and B-473, J-13, J-14 and J-15.

## Next

- **J-16, the known issues**, on the user's "proceed" (`milestone` skill; the list in `docs/ideas/ITERATION-1.5.0.md`, phase 8). First
  the user's look at J-15's frames (`shots/tests/kinds/desert_<orbit_before|orbit|sheet|ground>.png`), J-14's
  (`shots/tests/clouds/venus_orbit_<before|after|night>.png`, `venus_sheet.png`, `acid_orbit_after.png`, `seed_page_clouds.png`) and
  J-13's (`shots/tests/cold/seed_page_<ground|deep>.png`, `cracks_<orbit_before|orbit_after|sheet|ground>.png`), and play (green a
  desert, Page Up on a venusian world's seed page, Page Down on a cold one's).
- **The 1.5.0 release** on the user's word (the `release` skill: pull request, tag `v1.5.0`, six zips; the Linux builds need Docker,
  asked first). First, the user's verdict on J-12's frames (`shots/tests/record/<catalogue|died_out|found_wild|animal_gone|life_page>.png`),
  J-11's (`shots/tests/kinds/`) and J-10's (`shots/tests/overgrowth/ruins_*.png`, the sounds `shots/tests/sound_squelch.wav`,
  `vents_dry`, `vents_wet`, `creaks`, `hair`, `tear`), the frames of the known issues' fixes (`shots/tests/overgrowth/ruins_*.png` with
  `ruins_night.png`, `shots/tests/fates/fate_soup_seas.png` and `fate_dyed_flock.png`, `shots/tests/ways/ways_climber.png` and
  `ways_shallows.png`, `shots/tests/kinds/`), the user's play of R-449 to R-453, and a commit of J-12, of B-447 to B-471, of R-449 to
  R-453, of J-13, of J-14 and of J-15.
- **Proof base:** `build/prove/j15` (46 items). Against j14: 43 of 46 the same; `seed_kinds` (the desert's recipe, four frames) and `unit`
  (the three checks: 592, 0 failures) J-15's own; `sweeps`' fog-seas frame moved once (KI-446) and is the same run alone. `bench check` 54
  of 54 budgets.

## Playing the lab

In the laboratory, E opens a device. Microscope: vials by world and cultures by folder; on a slide L light, M magnet, H warmth, F food,
K keeps a cell. Incubator: flasks (a digit types the warmth, N names, K keeps a copy); a culture at its best is full in 2 to 4.5 minutes
and stays full (R-449). Sequencer: reads a genome in ten seconds. Tree of kin: the screen on the port wall. Gene editor: K puts a kin
beside it, M moves its gene in, D copies, S switches, Enter edits letters, C cuts, W writes a strain in ten seconds (folder, then name),
Page Down the world genes. R-450: change a gene (a letter, a cut, a move, a copy, a switch) and it is known for that species at once; the
line under the map says what it is for, and with its letters open (Enter) the map's place shows how its letters set it, their weights and
what each letter at the cursor would make of it. A changed vital gene would kill the strain: the editor says so and W refuses it. The
incubator's light field has UV. `lab microscope`, `lab cultures`,
`lab tree` and `lab genes` draw them into `shots/tests/`.

## Seeding a world

Grow a culture in a flask, park in orbit of a landable world (not home), E at the drone bay in the hangar, then S: C picks any culture
alive in a flask (B-473), the arrows move the place (Shift for ten degrees), Page Down takes it down a drill hole (J-13: the rock's 1 to 30
km, warmer down, dark, its food hydrogen; on an ice moon with a sea under its ice, that sea), Page Up up into the clouds of a venusian or
an acid world (J-14: 70 C down to 0 C, acid droplets, lit, dead matter), and the page shows what the culture needs and what the place gives
(R-451); Enter sends a drone. J-15: to green a desert, take a light-feeder with a green pigment, and on the editor set its DRY and BIND
heavier, cut its STOP (and its SHELL and LIFT if it has them: the cut STOP's line says what it will overgrow as, CARPETS or LICHEN), tune
its WARMTH to the desert's, and send it to the desert's land: a carpet spreads, grass and shrubs follow four steps behind it, and from
its 27th step (half an hour or more) the data sheet names a GREENED DESERT. J-14: a culture from a venusian world's clouds (the capsule's scoop
takes them on the way down) lives there; water life never; its dye streaks the clouds and its glow lights their night. J-13: a cold world
takes a line that lives in brine with its FROST cut (the editor: SALT heavier to BRINE, FROST cut, WARMTH lighter, down to -45 C), or a
hydrogen eater sent down to where the rock is warm. Its report comes about two and a half minutes later, with the reason in full if it
died, and the drones can go again at once (R-449). A dye line (GREEN and PINK too, R-453) turns the waters it lives in its colour within a
minute or two near the place and over the whole world in fifteen minutes to an hour (R-449: four times as fast as before); land to see it,
look from orbit or open the landing map. A line that gives off a gas (set its GAS gene on the editor's second page) changes the air from
about eight minutes in to half an hour: land to read TEMP and ATM, or open the data sheet's AIR line. A line with LIFT carries its dye,
glow, frost or spores into the clouds. A line's other world genes act where it lives: SHELL (chalk, glass, rust, sulphur), SLIME, BIND,
STOP cut, DRY, ROT (wood, stone), and HOST set to plants with its dye, glow, frost or CARRIER. The data sheet's second page has SEEDED,
MADE and AIR. `seed worlds`, `seed sea`, `seed climate`, `seed traits` and `seed looks` show the verdicts, the seas, the climates, every
made air and every made water, ground and plant in `shots/tests/`. J-09: a line with its STOP cut and its BIND set overgrows the land it
lives on from its eighth step on (a SHELL of chalk makes coral, of glass a sponge; LIFT bladders; SLIME without BIND films by the water):
land by the place to walk among it, or watch its stain spread from orbit. J-10: let it go by a town of a people's world and land there
later: the cover climbs the walls, the doorways grow shut (walk through them), and a wet one squelches underfoot; a line with ROT set to
stone crumbles the ruins it reaches. J-11: once a world's change passes a line, the data sheet names its made kind; a CO2 line on a warm
wet world runs away and dries its seas (MADE HOTHOUSE), a dimethyl sulphide line on a tundra freezes it (MADE SNOWBALL), a CO2 line on a
tundra greens it. `seed kinds` draws each kind into `shots/tests/kinds/`. J-12: look at a world's species under the microscope before
seeding it with a POISON line, and after about half an hour (the line over the whole world) open the catalogue: those it drove out read
DIED OUT. Sample the world again and put the line's vial under the microscope to find it wild, then read it on the sequencer to see what
the wild changed in it. The statistics' L page counts the worlds seeded and what died out. The known issues' fixes: let go a red dye line
and later a blue one on the other side of a world, and each sea keeps its own colour from orbit and on the ground; a methane line on an
oxygen world raises silver clouds in the summer twilights of the high latitudes; a CO2 line on a tundra melts its ice sheets' snow to bare
ice; a CO2 line on a warm ocean or hycean world can boil its sea off to a salt floor (MADE HOTHOUSE, ONCE HYCEAN, in the analyzer too); a
second line let go on a warmed world reads its warmth (a vial there too).

## Seeing the animals

Land on a world with plants: herds graze in the open, hunters stalk them, flyers circle overhead and perch in trees at dusk, swimmers
break the surface of seas and deep lakes, and floaters drift on the wind. X marks the creatures nearby. Put the crosshair on an animal
within 40 m (its words show over the stamina, "A STRIPED HOPPER - SHIFT+N") and Shift+N names it (R-452; N alone is the map); the data
sheet's LIFE line lists what you saw.
`bestiary ways` draws each way, and every land body's walk and run, into `shots/tests/ways/`. J-08: seed a world with a line that harms (a
working POISON, a HOST of animals, a sulphide GAS: the editor's second page), wait a quarter of an hour or so and land by the place: remains
lie where the herds lived and by the water, flies over the fresh, the sick and the changed among the living; X and the data sheet's LIFE
line say DYING, GONE or CHANGED. `seed fates` draws each fate, each change and a herd over its steps into `shots/tests/fates/`. `bestiary
plates` draws 64 worlds' animals one by one and a herd each into `shots/tests/plates/` (`plate_NN.png`, `herd_NN.png`, all of them small in
`plates.png`).

## Playing the ship

Shift runs, Page Up takes the lift, U dims the lights, Y makes the hull glass. E works the bunk, the desk, the music player's deck, the
capsule, the quarters' wall of photographs and a garden bed. The buggy and the drone stand in the maintenance room astern of the hangar.
On the ground O takes a sample, Shift+O shows the activity vision, Shift+U the ultraviolet lamp. `ship rooms`, `ship light`, `ship life`
and `life kit` draw them.

## Open with the user

- **J-07's calls:** swimmers of the shallows are rare (KI-467); naming moved to Shift+N (R-452).
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
- **R-448's calls:** the plan's defaults hold (the drones send a culture, the animals in the middle, home never seeded, the explorer
  names the animals) until the user says otherwise; a world now takes half an hour, not two hours (R-449).
- **This batch's calls (R-449 to R-453):** the flask's times a fifth of what they were, all alike, so every verdict and pace is as before;
  a full flask fed for good, a starved one still resting or dying; the world's steps 75 s; the drones never charge; a gene known by its
  species once changed (an older guide's by genesis dropped); W refuses a dying line; the seed page shows facts, never the verdict;
  GREEN and PINK taken from the dye tables' synonyms. R-454 waits for the talk (its R file's five questions).
- **J-13's calls:** a brine line's floor -45 C (the talk's figure); the drill's eight depths to 30 km; a world's inner heat by its radius
  (30 C a km on an Earth, half again with plates, vents or tides); the deep's food the rock's hydrogen alone, so a hydrogen eater goes
  down; a deep line shows only its gases (KI-506), an ice sea's line its dye in the cracks; the two smallest cold worlds of `seed cold`
  stay closed (KI-505); a place read at its own warmth (it was read at -20 C however cold).
- **J-15's calls:** a desert is the desert kind alone (the scorched felisian lends its grass bank to its clay, and the other bare worlds
  keep a plain overgrowth: KI-510); a lichen greens as a carpet does; the plants start four steps behind the carpet's edge and thicken by
  twelve; they take the line's pigment's colour, a dye's over it (a purple line makes a purple desert, still GREENED DESERT), in the
  desert's own grass bank (two lines share it: KI-509); grass and shrubs, no trees, flowers or animals; the carpet harms nothing, but a
  soup the same line makes in the desert's pools still counts as harm; the editor's cut STOP names the overgrowth.
- **J-14's calls:** the clouds' heights by warmth (70 C down to 0 C, 8 C a km, from 3 km up: the scoop's lapse) so a player picks a
  warmth as down the drill; their food dead matter and light, as a vial of them (a cloud species that burns a food finds none: KI-507;
  the clouds could give a sulphur food instead, the user's call); water life never there; a cloud line's streaks and glow seen from
  orbit alone (KI-508); a warmth never said as -0 C.
- **Docs still in the old style:** the reference notes (`docs/reference/`, 1.4 MB) and the code comments, until each is next edited.
- Standing leave to commit each step; the clangd-lsp plugin; B-440's leftovers in their `shots/` (`probe_1`, `probe_2`).
- Known issues (`docs/iteration-3/bugs/KNOWN-ISSUES.md`): KI-424, KI-425, KI-428 to KI-431, KI-435 to KI-438, KI-440, KI-444,
  KI-446, KI-448 to KI-450, KI-453 (the data sheet calls a second sun's planet a moon), KI-454 (a heard sound's status line can run
  off the screen), J-12's KI-487 and KI-488 (a native driven out in warmths the line cannot live in too; the catalogue's animals by
  world alone, in words), and what the J series' fixes left, KI-489 to KI-503 (the overgrowth's lattice at the cube's edges and the
  cover's edge; the ground's cover at the landing's layer while the ruins move on; a ruin's glow in patches; a world's own crust in a
  line's form; kills kept a landing alone; a flock's changed young; a climber hugging a pole; a curving shallows; 13% of `seed kinds`'
  seedings showing nothing; a line's own warming moving its band but not its gas; an ice sheet's dome; a settlement's one line; five
  made colours a disc; a flask of a colour none near), R-449's KI-504 (a full flask still counts as starving for a WHEN STARVING
  switch), J-13's KI-505 and KI-506 (the two smallest cold worlds closed; a deep line seen only in its gases), J-14's KI-507 and
  KI-508 (a cloud species that burns a food finds none in its clouds; a venusian world's ground shows nothing of a cloud line), and
  J-15's KI-509 and KI-510 (two lines' grass in one colour; only the desert greens). J-16 takes the ones 1.5.0 found.
- **The known issues' calls:** a made colour takes the first free bank of a pool (the overgrowth first on a crowded world, then the
  waters, the hides, the looks, the foam, a dyed rain, then each other line's colour); where two lines lie the one of the greater share,
  the later on a tie; a living world's ice sheets keep their domes, their snow melting to bare ice; a later line is judged on the world
  the ones before it left, one way only; the shares were tuned by naming SILVER CLOUDS, greening cool worlds and counting a milder world
  more livable, not by giving every line more gas (that made more harm); an ocean's dried floor is mostly salt.
- Offered by B-430: the tables' dump as a harness mode for the container; parallel.h's misleading indentation (38 of the Linux builds'
  46 warnings). Offered by B-431: the `returnState =` lines before a scene's `openScreen`, now redundant.
- Our Docker images `vesperis-build:arm64` and `:amd64` were removed on the user's word. The old proof kits (`build/p00` to
  `build/d05`, about 9 GB) are deleted only on the user's word.

## Disk

About 52 GB free (2026-11-23). `build/prove` keeps the snapshots r434, r438, r442, b443, y01, y02, y03, y04, r446, r447, j01, j02,
j03, j04, j05, j06, j07, j08, j09, j10, j11, j12, b471, r453, j13, j14 and j15, the compares' outputs against r434 to j14, and their
logs: about 77 GB. A snapshot and a compare's outputs are about 1.4 GB each.

## Next free ids

Bugs B-474, requests R-455, known issues KI-511. Milestone ids go by letter series in `docs/ideas/MILESTONES.md` (1.5.0 took Z, I,
Y and J).
