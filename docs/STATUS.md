# Status

The state of the work, in one place; each milestone's close rewrites it (`docs/CLAUDE.md`).

- **Released:** 1.4.0 on 2026-10-29, the branch `release/v1.4.0` (from main 9c4ebc2) merged into main by its pull request (#9)
  and tagged `v1.4.0`, with its six zips (`packaging/RELEASE.md`).
- **In 1.5.0,** the small and the spacious, on the branch `release/v1.5.0` (from main bb0e58b, made 2026-11-16): the iteration
  `docs/ideas/ITERATION-1.5.0.md`, its design `docs/ideas/IDEAS-small-and-spacious.md`, on the requests R-425 (microorganisms on
  every world, searched for and studied aboard) and R-426 (a Stardrifter with room, not utilitarian). Eleven milestones: Z-01 and
  Z-02, I-01 to I-04, Z-03 to Z-07. Done: Z-01 (2026-11-16), where life is: `src/life/` (`docs/reference/13-life.md`), every
  landable world's stage of life, chemistry, genesis and species; `GEN_VERSION` 17, nothing of 16 moving. Z-02 (2026-11-16), what it
  looks like: every species' form (`life/morphs.h`) and the slide it is seen on in six ways of seeing (`life/morph_draw.h`), its
  life cycle's forms and seventeen acts; `life plates`, `strips`, `distinct` and `forms` draw them. I-01 (2026-11-16), the ship as
  rows: `game/ship_plan` (rooms, the lifter, fixtures, colliders, lamps, views; the mesh built once, projected once a frame and
  filled by the bands), the 1.4.0 cabin its first plan, the walk, auto-explore's places and the harness's views read from it;
  nothing drawn, played or saved changing. I-02 (2026-11-16), the new Stardrifter: three decks and eight spaces on the rows (the
  bridge, the hall with the spiral stair, the library, the laboratory, the quarters, the hangar, the engine room, the observatory),
  the arches, the stair walked round, the lift; the devices in their rooms; the rooms seen through the arches drawn in their boxes
  of the screen, the space view skipped with no window in view, the banks dealt by nearness; auto-explore's routes; `ship rooms`
  and `ship walk`; `cabin_1x` blessed (the bridge). I-03 (2026-11-16), light, materials and views: every surface a finish (warm
  panels, a darker floor, fabric, brass, plants, glass, the hull's grey) dealt its bank by the frame, each room's lamps as pools on
  its surfaces cut to a 1 m grid (U dims them), the sun through the windows and the dome as patches with the boxes' shadows, the
  parked world's light on the walls facing it, the hull outside (the roofs, the prow, the drive's ring, the radar's mast, the drones'
  door) lit by the sun; `ship light`; `cabin_1x` blessed again. I-04 (2026-11-17), life aboard: the quarters' bunk (the almanac's
  rest), desk (the log) and wall of the last twelve photographs printed in one tone; the hall's garden of the home world's flora
  grown by the days played, and the drive's core under the floor glass brightening with a flight; the library's shelves of shards
  aglow by how many are held, the case's instruments, the player's deck; the capsule as the surface draws it, its drop through the
  hatch when boarded at the capsule; U's night keeping the reading lamps; each room's sound (`sounds`); `ship life`; R-426 done.
  Z-03 (2026-11-17), the search: a sample of the ground at the crosshair (O on foot, eight vials a descent), its microhabitat read
  from what the world fixes at its square metre, its line with the temperature and the activity read on the spot; what it holds a
  pure function of the place and the biota (`life/samples`, `surface/sampling`); the capsule's scoop in a venusian or acid world's
  clouds; the vials to the cold store (64 places, the save's lines) at the climb aboard; the lamp's ultraviolet (Shift+U), the living
  films glowing; red snow on cold living worlds without a people and the hot springs' mats in bands by their heat (generation 17
  still); `life search`, `life kit`. Z-04 (2026-11-17), the microscope: the lab's first device on its stern bench (`E`, `MICROSCOPE`, a
  camera entered), the cold store's vials (Enter on the stage, Backspace empties one; the store's door lit by eighths), a vial's slide
  (`life/slides`: its mount a pure function of the vial, prepared by tens, every species a tile of its own; what is done to it closed-form
  in its clock: a spot of light, the magnet turned, the stage warmed, food) in six ways of seeing, zoomed, focused through the drop,
  a cell tracked; what is seen of a species the guide's (`microbe` lines), a form's first sight logged (FORM), its sheet (Tab), its name
  (N), a micrograph (P); `lab microscope`, `lab time`. Z-05 (2026-11-17), the cultures: the lab's second device, the incubator on
  the stern bench (`E`, `INCUBATOR`, a screen over the room): twelve flasks, each a medium (its cell, warmth, light, energy, salt; AS
  ITS WORLD the first guess) and one or two species seen in the vials or carried from a flask; a culture closed-form from its events on
  the lab's clock (`life/cultures`: its lag, its rise, its crowd, its decline into spores, cysts, fruiting or lysis; spores woken with
  their years; a luminous crowd's glow), two in a flask (one wins, they cycle, they feed each other, mirrored they ignore each other),
  strains at a range's edge (named with N); the jars' shelf (`life/jars`: a world's silt settling into its bands over twenty minutes);
  the flasks and jars lit in the room, the microscope's drop of a culture, the sheet's stages and growth; `lab cultures`, `lab jars`.
  Z-06 (2026-11-17), the genome and the tree: a species' genome and lineage in closed form (`life/sequences`: its genesis's alphabet and
  hand, a marker shared with its kin as far back as their common ancestor; kin by world, by system and by the kin cell's boxes, none across
  geneses); the lab's sequencer on the bow bench (`SEQUENCER`: a species of a vial or a flask read in 90 s on the lab's clock, its letters
  under a chromatogram, its kin told and logged, KIN) and the tree of kin's screen on the port wall (`TREE`: every species read and the
  ship's own by kinship, the geneses counted, a kin's star the remote target; a glowing tree on the wall); on the late peoples' worlds a
  made species whose watermark is a record of its people (`peoples/made`, the grammar's `watermark`), read on the decoder after their
  records; `lab tree`, `lab kin`, `life made`. Z-07 (2026-11-17), the record and the close: the guide's catalogue (`CATALOGUE`: every
  species seen, by world or by kin, a sheet beside its form drawn live, named from it), the statistics' LIFE page and the data sheet's
  microbe line (every count the explorer's own, in the other screens' words), samples counted and logged (SAMPLE), a friend's species'
  sheets lent in cyan and in no count, auto-explore's samples at the water and the springs; the cookbook's recipes, the reference
  notes; `lab catalogue`, `life auto`. R-425 and R-426 are done: all eleven milestones. Their entries are
  `docs/decisions/log/7-1.5.0.md`, their rules `docs/decisions/rules-life.md`, `rules-life-lab.md`, `rules-life-cultures.md`,
  `rules-life-record.md`, `rules-game.md`,
  `rules-peoples-grammar.md` and `rules-ship.md`. Committed on the user's word (2026-11-18) with B-434 to B-438 below (0c90433),
  B-439 after it (455c7ad), B-440 with R-427 to R-434 below (82592c5, 2026-11-19) and R-435 to R-442 with B-441 and B-442 below
  (2026-11-19), each on the user's word: four commits on the branch, not pushed.
- **Fixed before the release** (2026-11-18, the user's reports in play): B-434, the laboratory's sunlight over its arch from the
  hall (a room's sun patches cast onto its surfaces' own pieces, round their holes); B-435, far things aboard grey until near (the
  ship's banks dealt once the outside is drawn: every bank the glass does not show; KI-439's flasks with it); B-436, a small moon
  lost from a day sky as the view turned (a point or a globe by its size at the view's middle). Then, on the user's word, the two
  known issues they left: B-437 (KI-441), the outside seen through the glass in its own palette, so the ship's banks never wait on
  what the glass shows; B-438 (KI-442), a world's season step kept by its seed, not its index (the harness's sweeps with it: every
  globe drawn with its map). Then B-439 (KI-443), on the user's word: the bench's ship rows budgeted again on the dressed ship
  (set on I-02's bare rooms, the library's had come to 1.27 times its median), each timed once the maps its view starts are made.
  Then B-440, found reading the user's play files: a unit run from the repo root had written a test's guide over the user's
  `guide.txt` (lost: a check's second game kept the game's paths); a test game's files all default under `shots/tests/`, the unit's
  `files` group holds the player's (KI-421's probe images with it). Their B files, and entries in `docs/decisions/log/7-1.5.0.md`.
- **Requests aboard** (2026-11-18, the user's after walking the ship; R files and entries in `7-1.5.0.md`): R-427, the maintenance
  room (the engine room's ring, the "weird portal", gone; the buggy and the drone on platforms, the ground's shapes); R-428, the
  quarters' wall hung by the explorer (`E` at it); R-429, the garden's beds grown by the time played, trimmed back by `E`; R-430,
  the ship quiet but its hum (I-04's room sounds gone: 1.4.0's audio again); R-431, the decoder and the player apart (the player
  its own screen at the deck, a folder a world, music alone).
- **Requests for the laboratory and the search** (2026-11-19, the user's, talked through first; R files and entries in `7-1.5.0.md`):
  R-432, the vials by world (the microscope's lists in two tabs, the vials a folder a world; the cold store without limit, the kit's eight
  kept; the incubator's and the sequencer's choosers in folders); R-433, the culture collection (every species and strain a flask grows
  kept, worked as a world's: on the stage, into a flask, on the sequencer; the explorer's folders); R-434, the activity vision (Shift+O:
  the ground and the water within 40 m by the activity a vial would read there, a thermal camera's way).
- **Requests for the laboratory's experiments** (2026-11-19, the user's after playing R-432 to R-434, talked through first and built on
  the recommendations; R files and entries in `7-1.5.0.md`): R-435, the warmth typed (a flask's by its digits, the microscope's stage by H,
  Shift+H the vial's again); R-436, the magnet made clear (M beside the stage, turned, taken away; drawn with its poles and its field's
  lines); R-437, the light and the food made clear (the HUD's panel of the four things done, what each is for said, what the view's cells
  answer told and kept on their sheets, the guide's `answers`); R-438, the answers made distinct (every turner within its reach answers,
  the dark's swimmers flee a bright light, a kind moves only in its range of warmth at its warmth's pace, lit light-feeders draw the
  eaters). With them B-441, found measuring their slides: a culture grown on its partner kept with it (their two acid-lake cultures
  repaired).
- **Requests for the incubator and the collection** (2026-11-19, the user's after asking what the jars are for, talked through first and
  built on the recommendations; R files and entries in `7-1.5.0.md`): R-439, the jars made clear (the bands by what each lives on, never
  over another, an eater that needs air at the air's edge; numbered and named on the page, kept on the sheets, the settling said); R-440,
  the collection by hand (K keeps a copy in a folder: the cell under the microscope's crosshair alone, a flask's one or two; C copies; a
  pair one entry); R-441, the choosers' two tabs (THE VIALS, THE CULTURES; the sequencer's too); R-442, naming in the incubator (N). With
  them B-442, found in their save: a vial's unnamed species read alike in the lab's rows (now each its look).
- **The ship to walk:** `vesperis_test ship rooms`, `ship light` and `ship life` draw its spaces, their light and what is kept in them
  into `shots/tests/ship/`, and the game walks it (Shift runs, Page Up the lift, U the light, Y the glass hull, E at the bunk, the desk,
  the deck (the player), the capsule, the quarters' wall (its photographs hung) and a garden bed (trimmed); the buggy and the drone in
  the maintenance room astern of the hangar). On the ground O takes a sample, Shift+O the activity vision, Shift+U the ultraviolet;
  `life kit` plays a landing's kit, `life sight` the vision. In the laboratory E at the microscope opens its lists (the vials by world,
  the cultures by folder; on a slide L the light, M the magnet, H a warmth typed, F food, K keeps a cell), E at the incubator its flasks (a
  digit types a flask's warmth, N names, K keeps a copy) and the jars, E at the
  sequencer a genome's reading, E at the tree's screen the tree of kin; `lab microscope`, `lab cultures`, `lab jars` and `lab tree` draw
  them into `shots/tests/`. The guide's THE CATALOGUE lists every species seen and the statistics' `L` their LIFE page (`lab catalogue`).
- **The plates judged:** the user's verdict on Z-02's plates (`life plates`, `strips`, `forms`), 2026-11-16: "microbes pictures you
  made today looked great"; Z-04 builds the microscope on them as they are.
- **In 1.4.0:** the sixteen world milestones, P-00 to P-07, Q-01 to Q-03 and D-01 to D-05 (R-421 to R-423: the kinds, the traits,
  the drifters); the reorganisation's thirteen, E-01 to E-13; the U series, a screen in one row (R-424), U-01 to U-06; and B-429 to
  B-433, the last four the known issues KI-423, KI-432, KI-433 and KI-434, fixed on 2026-10-29. Each has its entry in
  `docs/decisions/log/6-1.4.0.md` or its B file. The iteration, its ideas and its two plans moved to `docs/archive/ideas/`
  (`ITERATION-1.4.0.md`, `IDEAS-worlds-and-wanderers.md`, `PLAN-reorganisation.md`, `PLAN-screens.md`).
- **Next:** 1.5.0's iteration is done; the release (the `release` skill: the branch merged by its pull request, the tag `v1.5.0`, six
  zips; the Linux builds need Docker, asked first) on the user's word, and commits only when asked.
- **The proof's base:** `build/prove/r442` (35 items: r438's, with R-439 to R-442's and B-442's checks and frames: the unit's `incubator`
  group, the jars' bands numbered and named, the lab's rows in looks, the tour's N, K and the choosers' tabs, its 489 pictures).
- **Open with the user:** standing leave to commit each step; the clangd-lsp plugin; B-440's leftovers in their `shots/` (the
  test's `probe_1` and `probe_2`, theirs to remove); KI-424 (the grammar's picture register, which
  no call expands); KI-425 (the radar's heard list); KI-428 (a T typed at the console warps time); KI-429 (the settings' title over
  their first row); KI-430 (a race in the unit's lent piece frame); KI-431 (the almanac's STOP THE RUN, offered and ignored); KI-435
  (aboard, the local target's HUD label over the ship's walls, found in I-04); KI-436 (a chemotroph world made from a hot tectonic one reads
  188 C on its ground, found in Z-03); KI-437 (the microscope's electron view over large cells, 25-47 ms a frame at 4x) and KI-438 (Z-02's
  plates draw spores moving as their cells), found in Z-04; KI-440 (the data sheet's GRAVITY line past the frame's edge), found in
  Z-07; KI-444 (`surface 4x` the bench's tightest budget, 1.30 times its median), found by B-439; KI-446 (the battery's `rogues` and
  `sweeps` items each differing once and the same alone), found in R-438; KI-447 (the tree's leaves still UNNAMED alone), found in
  B-442; offered by B-430: the tables' dump as a harness mode for the container, and parallel.h's misleading indentation (38 of the
  Linux builds' 46 warnings); offered by B-431: the `returnState =` lines before a scene's `openScreen`, now redundant; our Docker images
  `vesperis-build:arm64` and `:amd64` (removed on the user's word); the old proof kits (`build/p00` to `build/d05`, about 9 GB),
  deleted only on the user's word.
- **The disk:** about 127 GB free (2026-11-19: the old snapshots and their compares' outputs went on the user's word, then r438 and r442
  were made; `build/prove` keeps r434, r438 and r442 and the compares' outputs against r434 and r438, 2.6 GB, with their logs); a snapshot
  and a compare's outputs about 1.3 GB each (`life_look`'s 60 pictures are 0.2 GB of it).
- **Next free ids:** bugs B-443, requests R-443, known issues KI-448. Milestone ids go by letter series in
  `docs/ideas/MILESTONES.md` (1.5.0 took Z and I).
