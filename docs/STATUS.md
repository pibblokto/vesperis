# Status

Where the work stands. Each milestone, bug or request batch rewrites this file (`docs/CLAUDE.md`). What each item did is in its
log entry (`docs/decisions/log/`), B file or R file.

## Releases

- **Released:** 1.5.0, "the small and the spacious", on 2026-11-23: the branch `release/v1.5.0` (from main bb0e58b) merged into main
  by its pull request and tagged `v1.5.0`, with its six zips (`packaging/RELEASE.md`). `GEN_VERSION` 18. The next release is not
  planned: its plan will be an `ITERATION-<version>.md` in `docs/ideas/`, and its entries start a new log file,
  `docs/decisions/log/8-<version>.md`.
- **In 1.5.0:** microbes on every landable world (Z-01 to Z-07, R-425; generation 17); the new Stardrifter (I-01 to I-04, R-426); the
  genes, the gene editor and the trees by world and by folder (Y-01 to Y-04, R-443, R-444); seeded worlds (J-01 to J-12, R-448;
  generation 18 from J-06); a culture for any world (J-13 to J-15, R-454); the known issues 1.5.0 found, fixed as B-474 to B-507 (J-16);
  the player's requests R-427 to R-442, R-445 to R-447 and R-449 to R-453; bugs B-434 to B-507 (B-495 unused); and, before the release,
  `<cstdlib>` in the fourteen game files that call `atoi` or `getenv` (the Windows builds failed without it). Each has its entry in
  `docs/decisions/log/7-1.5.0.md` or its B or R file. The iteration and its three ideas files moved to `docs/archive/ideas/`
  (`ITERATION-1.5.0.md`, `IDEAS-small-and-spacious.md`, `IDEAS-genes.md`, `IDEAS-seeded-worlds.md`).
- **Next:** the user's next request; nothing is planned.
- **The proof's base:** `build/prove/j16` (46 items, 3,808 s). Against j15: 18 of 46 the same, regress's 24 hashes among them; 28 differ,
  each by its fix (the log's J-16 entry). The release's code adds two things after it: the ground's night light for a glowing deep line's
  cracks (B-479: eleven items compared against j16, all the same) and the `<cstdlib>` includes (the game's objects the same).

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
its 27th step (half an hour or more) the data sheet names a GREENED DESERT. J-14: a culture from a venusian world's clouds (the
capsule's scoop
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
second line let go on a warmed world reads its warmth (a vial there too). J-16: a sea line on a terminator world now changes its air; on
a small cold world Page Down goes deeper (a 767 km rocky world opens 41 km down); a dyed line sent down a drill hole stains the bare rock
round its seeps (RED SEEPS), and a glowing one under an ice moon's crust lights its cracks at night; an acid world's iron or sulphide
eater grows in its own clouds (the seed page says IRON, SULPHIDE); dye or light a venusian world's clouds and land: its sky takes the
dye by day and glows in patches at night; green a desert with two lines of two colours, each keeping its own, or a scorched felisian;
warm a tundra and its ice sheets' white draws back behind a dark margin; on the ground a hunt's kill stays dead across landings and saves.

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

- **Frames still waiting for the user's look:** J-16's (`shots/tests/j16/`: the venusian ground under a glowing cloud line by day and
  night, a desert greened in two colours, red seeps, an ice moon's glowing cracks at night, each beside its before) and the battery's
  (`build/prove/j16/items/<item>/shots/tests/`: the data sheets, the catalogue by kin, the ruins at night, the climber and the shallows,
  the cabin's marks); J-10 to J-15's (`shots/tests/overgrowth/ruins_*.png` and its sounds, `kinds/`, `record/`, `cold/`, `clouds/`).
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
  (its warmth read too since B-474), and then leaves every vial of the world; the catalogue lists the animals seen by world, with sheets in
  words (by kin and drawn since B-500); DIED OUT counts the explorer's own sheets alone; what the wild changed shows once the sequencer
  read the line from a
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
  down; a deep line shows only its gases (seeps and glowing cracks since B-479), an ice sea's line its dye in the cracks; the two
  smallest cold worlds of `seed cold`
  stay closed (open since B-478); a place read at its own warmth (it was read at -20 C however cold).
- **J-15's calls:** a desert is the desert kind alone (the scorched felisian lends its grass bank to its clay, and the other bare worlds
  keep a plain overgrowth; any bare world with air greens since B-482); a lichen greens as a carpet does; the plants start four steps
  behind the carpet's edge and thicken by
  twelve; they take the line's pigment's colour, a dye's over it (a purple line makes a purple desert, still GREENED DESERT), in the
  desert's own grass bank (two lines share it; each its own since B-482); grass and shrubs, no trees, flowers or animals; the carpet
  harms nothing, but a
  soup the same line makes in the desert's pools still counts as harm; the editor's cut STOP names the overgrowth.
- **J-14's calls:** the clouds' heights by warmth (70 C down to 0 C, 8 C a km, from 3 km up: the scoop's lapse) so a player picks a
  warmth as down the drill; their food dead matter and light, as a vial of them (a cloud species that burns a food found none, KI-507;
  B-480: an acid carries iron and sulphur); water life never there; a cloud line's streaks and glow seen from
  orbit alone (from the ground too since B-481); a warmth never said as -0 C.
- **J-16's calls (the user's word on two):** KI-502's cap (a disc shows five or six made colours, past them the nearest's) and KI-503 (a
  flask past the ship's banks with no colour near dims to the glass's tone; the banks go to the nearest things first, so it is a far
  one) are the palette's 32 banks' limits: proposed closed, open until the user agrees. KI-436's fix changes which worlds are
  chemotrophs (17 of 65 near the start are their drawn kinds again) and rides generation 18 by the rules (a bump a series); the traits'
  and the cell genes' pinned digests moved with it alone (a build with it undone gave both old values). KI-507: an acid medium carries
  iron and sulphur in a flask too, so an acid world's iron or sulphide eater grows in any acid flask (J-14 noted "the clouds could give
  a sulphur food", the user's call: taken so). A line's share grows where its own warmth opens its world and never falls (its gas
  stays), so a hothouse still runs away. A line's gas answers by its room: harm is now 57% of `seed kinds`' seedings (KI-520). An ice
  sheet keeps its heights: its margin melts dark. A small cold world's drill goes to 120 km. One bank a greening line, its carpet its
  grass's colour, and GREENED DESERT for any bare world greened. Kills kept in the guide 64 steps (`dead` lines). A climber takes only a
  trunk as thick as its body (KI-526). The ground's resample after a step takes 45 frames (the one that waits for it about 34 ms on a
  busy machine). The catalogue's animals sit under NOT YET SEQUENCED until one of their world's species is read. The ground's budgets
  went to 1.5 times their medians.
- **Docs still in the old style:** the reference notes (`docs/reference/`, 1.4 MB) and the code comments, until each is next edited.
- Standing leave to commit each step; the clangd-lsp plugin; B-440's leftovers in their `shots/` (`probe_1`, `probe_2`).
- Known issues (`docs/iteration-3/bugs/KNOWN-ISSUES.md`): KI-424, KI-425 and KI-428 to KI-431 from before; of 1.5.0's, KI-502's cap and
  KI-503 (proposed closed, above), and what J-16's fixes found, KI-511 to KI-526 (a harness game at another scale, the overgrown city's
  tight budgets, scenes not waiting for their maps, auto-explore's second sun's planet, the catalogue's swimmers on dried seas, a moving
  electron stage, herds keeping the landing's fates after a step, a crossed ruin face's one light, two covers of one material sharing a
  tile, `seed kinds`' shares untuned, a line's room unsampled, seeps not by where water rises, a deck world's dye one tint from below, a
  sprouting flyer fallen out of sight leaving no remains, a night-born flyer by day, a climber with no thick trunk near).
- **The known issues' calls:** a made colour takes the first free bank of a pool (the overgrowth first on a crowded world, then the
  waters, the hides, the looks, the foam, a dyed rain, then each other line's colour); where two lines lie the one of the greater share,
  the later on a tie; a living world's ice sheets keep their domes, their snow melting to bare ice; a later line is judged on the world
  the ones before it left, one way only; the shares were tuned by naming SILVER CLOUDS, greening cool worlds and counting a milder world
  more livable, not by giving every line more gas (that made more harm); an ocean's dried floor is mostly salt.
- Offered by B-430: the tables' dump as a harness mode for the container; parallel.h's misleading indentation (38 of the Linux builds'
  46 warnings). Offered by B-431: the `returnState =` lines before a scene's `openScreen`, now redundant.
- Our Docker images `vesperis-build:arm64` and `:amd64` (made 2026-10-27, used again for 1.5.0's Linux builds), the old proof kits
  (`build/p00` to `build/d05`, about 9 GB) and `build/release-1.4.0` are deleted only on the user's word.

## Disk

About 40 GB free (2026-11-23) before the release's builds (`build/release-1.5.0`, about 1 GB). `build/prove` keeps the snapshots
r434, r438, r442, b443, y01, y02, y03, y04, r446, r447, j01 to j16, b471 and r453, the compares' outputs against r434 to j15, and their
logs: about 81 GB. A snapshot and a compare's outputs are about 1.7 GB each. J-16's five worktrees (about 1.4 GB, under the job's
scratch directory) were removed at its close.

## Next free ids

Bugs B-508, requests R-455, known issues KI-527. Milestone ids go by letter series in `docs/ideas/MILESTONES.md` (1.5.0 took Z, I,
Y and J).
