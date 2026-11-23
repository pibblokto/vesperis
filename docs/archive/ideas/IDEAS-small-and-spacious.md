# Ideas: the small and the spacious (life too small to see, on every world; a Stardrifter with room)

Planned 2026-11-16 on the user's requests (`docs/iteration-3/plans/requests/R-425`, `R-426`), for 1.5.0 as two blocks of milestones
(`ITERATION-1.5.0.md`): **Z, the living small** (microorganisms: where they live, what they look like, how they are found, what is
done with them aboard) and **I, the interior** (the Stardrifter made spacious). Nothing here is built. This note holds the intent,
the lists and the science; the iteration file holds the milestones, their checks, their order and the decisions taken with it.

The request, in the user's words: "add microorganisms mechanic that would allow you to search for and investigate microorganisms
that can be found on any planet. They should also have some sort of complex procedurall look generation and in game animations
when you bring them to the ship and investigate. They should be on different stages, you can explore them, you should come up
with some interesting mechanics for what you can do with them on the ship. Also another thing I want you do include is expanding
stardridter so that it's way way more specious - right now it's "utilitarian" as one of the players described."

The rule for the living small, as for the worlds of 1.4.0: what lives, where, how it looks and moves and what it does in a flask
is what a real organism does, a field or a laboratory measured, or a published model proposes (section 4). Where the science is a
proposal (life in acid clouds, in methane, in ammonia, a second genesis), the screens say what is seen, never more. The ship is the
explorer's home: its rooms are the game's own design, not the original's cabin.

"Stages" is read both ways (the iteration's defaults): the stage a world's life reached (from chemistry that is not yet life to
colonies of many cells, and the spores and fossils of life that ended), and the stages of each organism's life (a spore, waking,
growing, dividing, a colony, a fruiting stalk, a spore again), which the lab lets the explorer watch and bring about.

## 0. What exists (generation 16)

* **Life on the ground is seen, never studied.** Of the 60 kinds (59 bodies and the companion star; 51 landable), six carry flora
  and fauna (felisian, hothouse, superhabitable, steppe, tundra, terminator) and two fauna alone (ocean, eyeball). The archean world
  is microbial (`KT_MICROBIAL`: mats on the wet flats, stromatolite domes in the shallows, 7-15 a 16 m cell), the chemotroph vent
  life (`KT_VENT_LIFE`: pale mats round its springs, sinter chimneys, spores glowing at night in bank 15), the lichen world its crusts
  and moss (`MAT_CRUST`, `MAT_MOSS`). Q-02's traits draw what microbes do in bulk: the hot springs' orange thermophile mats, the pink
  lakes' halophiles, the milky seas' glowing bacteria, the blooms, the reefs; a scorched world's old seabed keeps stromatolite fields.
  Flora has no species (ten silhouettes, a look of eight values a family). The bestiary's 2-4 land species, flyers and a swimmer are
  sighted and logged, their names generated, nothing kept past a landing (the data sheet's LIFE line lists them).
* **No age and no origin.** No body, star or biosphere has an age; nothing carries material between worlds. A drifter knows the star
  it left (`drifterPast`: 1,500-20,000 years ago, 6-340 ly back) but carries nothing from it. No life in a giant's clouds (X-03) and
  no sun-fed life without a sun (rules-galaxy-worlds.md).
* **The habitats the ground already models:** water by kind (sea, river, lake, wetland pool, rift lake; tar, brine and methane seas;
  seas frozen under their freezing points), heat (geysers, hot springs within 2.5 km, steam, spring, plume, jet and curtain vents,
  fumaroles), ice and snow, salt, brine and potash, lava, the ruins of the dead peoples, the site's temperature and pressure; not
  humidity, UV or a dose. Twenty-five materials, the archean's `MAT_MATS` and `MAT_STROMATOLITE` among them.
* **On foot** the explorer reads the crosshair's distance and a water flag, Tab's readings (a LIFE sensor for the nearest herd), the
  data sheet; carries nothing (a shard goes to the guide when taken, `E` within 2.2 m; nothing hints at one). The keys on foot: R, L,
  G, J, Tab, U (the helmet lamp, D-04), V, N, X, M, K, B, F, E, Q.
* **The ship** is one room of 5 x 5 x 2.4 m (`game/cabin.cpp`): a window band on every wall and the roof, a roof deck reached by a
  lifter, the capsule in a cage in the middle and eleven devices against the walls (the GOES console, the flight computer, the star
  map, landing and target screens, the decoder, the radar, the telescope, the instrument case and the drone bay). It is about 150
  quads drawn over the space view in bands (each band projecting every piece); looking out, the space view's 19 banks and the cabin's
  13 (8, 9, 10, 21-30) take all 32. A player called it utilitarian: a box of consoles, nowhere to be.
* **Clocks:** the drones' and the probe's run on real seconds while the game simulates (11 of the 29 states), saved with the slot;
  nothing runs while the game is closed. The synth is mono (22,050 Hz), so a room can have levels, not a place in the stereo.

## 1. The living small (Z)

### 1.1 Where life is: a world's stage

Every landable world gets a biota (`biotaOf`, on salts of its own): the stage its life reached, the chemistry it is built on, the
genesis it descends from and its species. The stages, an append-only list:

| Stage | What a slide shows | Where |
| --- | --- | --- |
| STERILE | grains and crystals; at most organics fallen from space, cooked | the furnaces (magma ocean, molten, chthonian, steam, glass and the lava worlds); the cold dead rocks of a lifeless system |
| PREBIOTIC | vesicles and droplets that swell, stretch into threads and split, with no genome | a comet's dust, the hydrocarbon worlds' shores, the tholins of the far cold's ices |
| SIMPLE | small cells: spheres, rods, curved rods, spirals, filaments, square plates, stalks | archean, chemotroph, snowball's brine veins, the europan and tiger-striped plumes' frost, hycean shores, salt and brine worlds, cloud life, dark oceans |
| COMPLEX | large cells: a nucleus, vacuoles, plastids, cilia, shells and skeletons | ocean, eyeball, lichen, desert, the oldest of the simple worlds |
| COLONIAL | colonies and the first of many cells: rolling spheres, plates, chains, branching threads, amoebae that gather and rise into stalks | every world with flora |
| DORMANT | spores and cysts alone, thick-walled, ornamented, still; some wake in a flask | the dead living worlds (frozen and scorched felisian), rust worlds, frozen drifters; spores carried by impacts to a living world's airless neighbours |
| FOSSIL | cells and mats turned to stone, seen in thin section | the old seabeds of the scorched and rust worlds, the deserts that were wet, the oldest rock of a living world |

A world can show two: a dead world its spores and its fossils, a living world its fossils in old rock. "Found on any planet": every
landable kind gives a sample something to show but the furnaces, whose samples show grains and at most cooked organics.

### 1.2 What it is built on: four chemistries

| Chemistry | Its solvent | Where | Its look | Sources |
| --- | --- | --- | --- | --- |
| WATER | liquid water, fresh to saturated brine, about 253-395 K | most | the forms our microbes take | Rothschild and Mancinelli 2001; Takai 2008; Mykytczuk 2013 |
| ACID | droplets of concentrated sulphuric acid | the venusian and acidic worlds' clouds at their temperate height | small and dense, each in its droplet; a cycle of rising and settling | Morowitz and Sagan 1967; Seager 2021 |
| AMMONIA | ammonia and water, about 195-240 K | the ammonia worlds' shores | slow, dark, walls like crystals | Bains 2004; Schulze-Makuch and Irwin 2008 |
| HYDROCARBON | liquid methane and ethane, about 90-100 K | the hydrocarbon worlds' lakes (one world in several; the rest prebiotic) | stiff, angular membranes, very slow | Stevenson, Lunine and Clancy 2015; doubted by Sandström and Rahm 2020 |

The three proposals keep their doubt in the words: the sheet says what is seen ("CELLS IN THE METHANE: THEIR MEMBRANES ARE NOT
OURS"), never that it is so.

### 1.3 The species

A world of life has 3 to 16 species (a dead world 1-4 kinds of spore, a living world's airless neighbour 1-2; prebiotic worlds hold
kinds of protocell, not species). Each species has:

* **a niche:** where it lives, among the microhabitats of 1.7, the rare species only in the rare places (a spring's hottest band, a
  plume's fall, permafrost);
* **a way to live:** light (by its pigment), chemistry (hydrogen, sulphide, iron, methane, ammonia), the dead matter of others, or
  others alive (a grazer, a predator);
* **its limits:** temperature (lowest, best, highest), salt, acid, a dose of radiation; a dormant form's years;
* **its life cycle,** a subset of: dormant (spore, cyst), waking, growing, dividing (fission, budding, many at once, fragments),
  exchanging, colony, gathering, fruiting, sporulating, dying;
* **its behaviour:** how it moves (not at all, runs and tumbles, helical swimming, gliding, twitching, crawling, rolling,
  corkscrewing), what it turns toward (light, food, a field, warmth), whether it glows when crowded;
* **its form:** a genome of about forty values (1.5), its lineage's style among them.

### 1.4 Whence it came: geneses and kin

A genesis is an origin of life. Its descendants share a chemistry, a handedness (left or right: a mirrored genesis is as likely as
ours), an alphabet of their code (four letters, six or eight: hachimoji DNA, Hoshika 2019) and a style of form.

* **Most systems' life is a genesis of its own.**
* **Within a system,** impacts carry rock between worlds (Melosh 1988; Mileikowsky 2000): two living worlds of a system share a
  genesis, and a living world's airless neighbours keep its spores in their shaded regolith and ice.
* **Between systems,** in cells of 64 sectors, one living system in five descends from its cell's oldest genesis (stars born in one
  cluster trade rocks before they part: Adams and Spergel 2005): kin stars, tens of light years apart.
* **A drifter carries its origin's life:** spores in its ice, chemotrophs in its dark ocean, kin of the worlds of the star it left.
  The explorer learns that only by sequencing both.
* **Home's genesis is pinned;** the ship's own species (the garden's soil, the explorer's skin) are of it and root the tree.

Kinship is learnt only by sequencing (1.8). Two species of one genesis show a common ancestor: within a world a few hundred million
to two billion years back, across a system's worlds about the last exchange, across stars about the cell's genesis. Two of different
geneses show none ("NO COMMON ANCESTOR"), and where their handedness is mirrored, a grazer of one cannot eat the other.

### 1.5 What it looks like: the form genome and six ways of seeing

**The plans,** by stage:

* protocell: a vesicle, one with many layers, a droplet; threads as it grows;
* cell: a sphere, rod, curved rod, spiral, corkscrew, filament, square plate (Walsby 1980), star, stalk, bud, a shape that changes;
* amoeboid: lobes, threads or a net;
* flagellate: one to eight flagella, an eyespot, a pellicle that flexes;
* ciliate: rows of cilia, a groove, a contractile vacuole, a stalk that coils;
* shelled: a silica box in two halves, pored in rows and ribbed, or a test built of the world's own grains;
* skeletal: a lattice sphere with spines (Haeckel 1904);
* chambered: chambers along a logarithmic spiral;
* armoured: plates, and two flagella, one in a girdle;
* colony: a rolling sphere with daughters inside it, a plate, a chain, branching threads;
* gathering: cells that stream together, then a slug, then a stalk with a head of spores;
* spore: a thick wall, smooth, spined, ridged or netted;
* fossil: a dark outline in rock.

**The values,** about forty, in groups:

* the outline: Gielis's superformula (a symmetry of 0-12 and three exponents: Gielis 2003), a rod's length and width, its
  curvature, a helix's pitch and turns, how far the shape wanders;
* the size: 0.3 µm to 2 mm by plan (Young 2006);
* the arrangement: single, pairs, chains, clusters, fours, packets, palisades, rosettes;
* the wall: its thickness and ornament (pores in rows, ribs, spines by count, length and branching, a lattice, plates), a capsule's
  halo, a sheath;
* the appendages: flagella (count, place, length, beat), cilia (cover, rows, length, the wave's length), pili, a stalk and its
  holdfast, pseudopods;
* the inside: a nucleus, vacuoles and their pulse, granules, plastids and their shape, an eyespot, gas vesicles, a chain of
  magnetosomes, refractile droplets;
* the pigment: chlorophyll-like green, rhodopsin purple (DasSarma and Schwieterman 2021), carotenoid orange and red, melanin
  brown-black, phycobilin blue-green, none; under a red dwarf, darker toward black (Kiang 2007);
* the motion, and the forms of its life cycle.

The lineage sets a style (a wall's mineral, a symmetry, a flagellum's kind) that kin share and a second genesis does not: a clade
looks like a family, a stranger looks strange.

**Six ways of seeing,** each showing what the real one shows (Murphy and Davidson 2012):

* BRIGHT (brightfield): a bright field, pigments in their colours, colourless cells faint;
* PHASE (phase contrast): grey, the cells dark in a bright halo, their insides clear;
* DARK (darkfield): black, the edges bright and the flagella seen (the one light way to see them);
* GLOW (fluorescence): black, a stain's blue on the DNA, the plastids' red, a luminous species' own light;
* ELECTRON (scanning): one cell in grey relief at x10,000 to x40,000, its pores, spines, pili and plates, turned with the mouse;
* SECTION (a polarised thin section): for fossils and rock, the minerals in interference colours and the fossils' carbon outlines.

### 1.6 How it moves and changes

Each animation comes from the species' values and a seed, closed-form in the slide's clock, so a frame replayed is the frame seen:

* motion at low Reynolds number (Purcell 1977): runs and tumbles (Berg and Brown 1972), helical swimming, gliding, twitching,
  amoeboid flow with pseudopods, a colony rolling, a spirochete's corkscrew; flagella beating as travelling waves, cilia in
  metachronal waves (Brennen and Winet 1977), a contractile vacuole's pulse, a stalk's coil;
* growth and division: a rod lengthening and pinching in two, a bud swelling from a pole, many cells at once from one, a filament
  breaking, a colony's daughters let out (Kirk 1998), a protocell's thread splitting (Zhu and Szostak 2009);
* dormancy: a spore forming bright inside its mother cell, then the mother's lysis; a spore waking (swelling, darkening, cracking,
  the new cell coming out); a cell rounding into a cyst;
* among others: an amoeba engulfing a cell into a vacuole that shrinks; two cells joined by a bridge; magnetotactic cells turning
  with the field (Blakemore 1975); phototactic cells crowding into a spot of light; a crowd's glow coming on (quorum: Nealson and
  Hastings 1979); social amoebae streaming in spiral waves to a mound (Tomchik and Devreotes 1981; an excitable medium, Greenberg and
  Hastings 1978) and rising into a stalk with a head of spores; a colony's branching front on a plate (Ben-Jacob 1994); lysis.

### 1.7 The search: where to look and how to know

Microbes are not seen: the explorer chooses where life would be, takes a sample and learns aboard what was in it.

* **The microhabitat** at the crosshair within 2 m is read from what the site already knows: the material, the water (in it, at its
  edge, its kind), the heat (a vent, a spring's band by its temperature), ice, snow, salt, shade, the ruins, the soil under flora, a
  plume's fall. A sample is named by it: "WET SILT AT A LAKE'S EDGE, 9 C", "SINTER AT A SPRING'S RIM, 74 C", "FROST FROM A PLUME'S
  FALL", "SALT CRUST", "OLD WALL", "REGOLITH IN SHADE".
* **The kit:** eight vials a descent, refilled aboard, on a key of its own (`E` stays the shard's). A sample is a pure function of
  its square metre: the same place gives the same sample. Its activity is read on the spot, as the Viking landers' labelled release
  read metabolism: `ACTIVITY NONE`, `FAINT` or `STRONG`. It is a hint, and it can mislead: spores read none.
* **The lamp's ultraviolet:** at night or in shade the helmet lamp's second light (Shift+U) makes living films fluoresce within its
  reach: mats, crusts, wet soil, snow algae, a shore's green line, a stromatolite's skin, and faintly the rock where endoliths live
  (Friedmann 1982). It is drawn in bank 15, the living glow's (the chemotroph's spores, the milky sea's light), by the biota's density
  at each point.
* **The signs** the ground already shows become places to sample: the mats, stromatolites, hot springs, pink lakes, blooms, milky
  seas, reefs, crusts, moss, chimneys and glowing spores. Two more, where the science and the banks allow: the hot springs' mats in
  bands by temperature, the hottest bare (Brock 1978), and red snow, snow algae on the cold living worlds' summer snow (Hoham and
  Remias 2020), a life trait in a trait bank on worlds without a people.
* **The capsule's scoop:** on a venusian or acidic world the descent takes the clouds' air at their temperate height, one vial; it is
  the only way to their cloud life.
* **Aboard:** at the climb aboard the vials go to the lab's cold store, and the HUD says how many.

### 1.8 The lab: what the explorer does with them

The laboratory is a room of the new ship (2.2) with four devices (the microscope, the incubator, the sequencer and the tree's
screen), each a screen or a camera you enter in its own colour, beside two fixtures: the cold store, whose door glows by how full it
is, and the jars' shelf.

* **The microscope**, a camera you enter, shows a slide from a vial or a flask:
  * the six ways of seeing (1-6), x40 to x1,000, and the electron's x10,000 to x40,000;
  * focus through the slide's depth, pan, and tracking a cell (the stage follows it; its sheet on Tab);
  * four things to do to a slide: a spot of light at the crosshair (phototaxis), the slide's magnet turned (magnetotaxis), the stage
    warmed (stress: cysts and spores), a drop of food (chemotaxis);
  * a micrograph to the gallery, and a name for the species under the crosshair (`TEXT_ENTRY`).

  The first sight of a form is its moment: the HUD's one line and the log's entry.
* **The incubator** holds twelve flasks.
  * **The media:** each flask has a temperature, a light (dark, white, red, blue), an energy (organic broth, hydrogen and carbon
    dioxide, sulphide, iron, methane, none) and a salt (fresh, sea, brine). The other chemistries have the cryostat (90-100 K,
    methane), the acid cell and the ammonia cell. The preset AS ITS WORLD, the sample's own world's air and ground, is the first
    guess.
  * **Growth** follows Monod (1949): a lag, the exponential rise, the stationary crowd, the decline. Under stress a culture
    sporulates or encysts; a luminous species lights its flask when crowded.
  * **Waking:** dormant spores wake when the medium suits them ("WOKEN AFTER ABOUT 12,000 YEARS": Cano and Borucki 1995; Vreeland
    2000, disputed; Shatilovich 2023).
  * **What the explorer sees:** the curve on the incubator's screen; the microscope opened on a flask shows the stage it is in.
  * **Two species in one flask:** one wins (Gause 1934), or predator and prey cycle (Gause's Didinium and Paramecium), or they feed
    each other; mirrored handedness ignore each other.
  * **Strains:** a culture held at the edge of its range long enough gives a strain of its own (experimental evolution: Lenski and
    Travisano 1994; Blount 2008), one value moved (a limit, a pigment, a size), the explorer's to name.
* **The jars:** a world's wet silt in a column becomes its world in a jar (Winogradsky's column; Rundell 2014). It settles in bands
  by what lives at each depth, coloured by its species' pigments, over twenty minutes. There is one a world on the lab's shelf, lit
  from within, seen from the room and glowing in the dark ship.
* **The sequencer** reads a species' genome in about 90 seconds, shown as it reads in its genesis's alphabet, and gives its place in
  the tree.
* **The tree:** every sequenced species by kinship, clades by genesis, the common ancestors' years, the worlds (or UNKNOWN). With a
  kin's world under the cursor, Enter sets the remote target (as a chart's record does). The geneses found are counted ("LIFE BEGAN
  AT LEAST 5 TIMES").
* **The made:** on the worlds of the peoples of the futuristic and far-future ages, one species is theirs: a genome too regular to
  have evolved, and in it a watermark (as JCVI-syn1.0's genome held its makers' names: Gibson 2010). The decoder reads it as a
  record in the people's tongue.

### 1.9 The record

* **Names:** nothing is named until the explorer names it, a species, a strain, a kind of protocell or a lineage. The screens say
  UNNAMED and the world's name (or UNKNOWN).
* **The sheet** fills as the species is seen: its form and size (any slide), its motion (a living slide), its flagella (DARK), its DNA
  (GLOW), its wall (ELECTRON), its stages (each one seen), its growth (a culture: the best temperature, the medium) and its kin
  (sequenced). A world's total is never shown.
* **Where it is kept:** the guide keeps what is known (`<body>/M<index>` keys, their sheets, the names, the sequences, the strains).
  The save keeps what runs aboard (the kit, the cold store, the flasks, the jars, the sequencer), on the drones' clock.
* **What shows it:** a catalogue in the guide (by world, by kin), a second statistics page (LIFE), the log's kinds (SAMPLE, FORM,
  WOKEN, KIN, STRAIN), the data sheet's microbe line, micrographs in the gallery. Lent species come in cyan.

## 2. A ship with room (I)

### 2.1 What the players see now

One room of 25 square metres holds eleven devices, the capsule's cage and the explorer, with a window band all round. Every device
stands against a wall within three steps; there is no other room to be in, nothing that is not a console, one grey hull bank under
one light. The roof deck is a 3.2 m platform reached by a key. It is a cockpit you can stand up in.

### 2.2 The new Stardrifter

Three decks, eight spaces, about 820 square metres of floor (25 now). The ship is 30 m from the bow to the engines, 28 m across the
library and the lab, 19 m from the hangar's floor to the dome's top. The hall is its middle: every room of the main deck opens onto
it by an arch, and the spiral stair in it joins the decks.

| Space | Deck | Floor (m) | Height (m) | Windows | Holds |
| --- | --- | --- | --- | --- | --- |
| The bridge | main, fore | 10 x 7 | 4.2 | the great window: a seven-pane arc across the bow, 0.5 to 3.8 m up | the flight desk; the star map, landing and target screens on a console arc; the GOES console in an alcove |
| The hall | main, middle | 12 x 12 | 8.4 (two decks) | the oculus: the dome's opening above the stair | the spiral stair (the hangar to the dome), the garden's beds, the lamps, the Vimana's core seen through a floor glass |
| The library | main, port | 8 x 8 | 4.2 | a long window on the port flank | the decoder's desk, the shards' shelves, the instrument case, the player's deck, a sofa |
| The laboratory | main, starboard | 8 x 8 | 4.2 | a long window on the starboard flank | benches, shelves for the jars, the cold store; the microscope, the incubator, the sequencer and the tree's screen (Z-04 to Z-06) |
| The quarters | main, aft | 10 x 6 | 3.4 | a wide window astern | the bunk, the desk, the explorer's photographs |
| The hangar | lower | 16 x 14 | 4.6 | the floor hatch, the drones' door | the capsule in its cradle over the hatch, the drone rack, the probe's launcher, the kit's locker |
| The engine room | lower, aft | 10 x 8 | 4.6 | none | the Vimana drive: a ring whose glow and hum rise with a flight |
| The observatory | top | a disc of 6 m radius | a glass dome, 6 | the whole dome | the telescope on its mount, the radar's set, a couch |

### 2.3 Walking, and the ways that need none

* **The decks** are joined by the spiral stair in the hall (down to the hangar, up through the oculus to the dome) and the bridge's
  two steps. Doors are open arches.
* **Walking:** 1.6 m/s, Shift 3.2 m/s. The floor plan's walls and the furniture's footprints are the colliders.
* **Nothing needs walking.** Every device keeps its key and its menu row, the landing screen still deploys the capsule, and the
  flight keys fly from anywhere aboard.
* **Facing:** the device in the room you stand in, within its reach, most ahead.
* **Where the explorer is:** a new game starts on the bridge facing the window; the climb aboard returns them where they left from;
  the save keeps the room and the place.
* **Auto-explore's places** become the plan's: the bridge's window, the dome (the roof deck's heir), its routes through the arches.
* **The classic cockpit camera** stays the SHIP VIEW setting.

### 2.4 Light, materials and views

* **Materials** beyond the grey hull: warm panels, a darker floor, fabric, brass, plants and glass, each a bank dealt by the frame
  (2.6).
* **Lamps,** room by room, as pools on the floors and walls: the bridge dim for the view, the library warm, the lab bright, the
  quarters low, the hangar's work lights, the dome dark for the eyes. U dims the whole ship, as now.
* **The star's light** comes through the windows: each window's patch on the floor and the walls, moving as the ship turns. The parked
  world's light, by its phase, falls on the walls that face it.
* **The views:** the bridge's great window, the long windows on the flanks, the quarters' astern, the oculus, the dome. The hull is
  seen through them: the bow's curve, the radar's mast turning while it sweeps, the drive's ring glowing aft, the drones' door. Y
  depolarises every outer wall.

### 2.5 Life aboard

* **The quarters:** the bunk (rest: the almanac's run to an event), the desk (the log), and the explorer's photographs on the wall,
  the gallery's last twelve printed in one tone.
* **The hall's garden:** the home world's plants in beds, swaying in the air system's draught and growing over the days played; the
  lamps' warm light; the Vimana's core through the floor glass.
* **The library:** the shards on shelves, lit by how many are held, the instruments behind the case's glass, the player's deck and a
  sofa by the window.
* **The hangar:** the capsule in its cradle (the capsule the surface draws), its hatch opening as it drops; the drones on their rack
  by their door; the kit's locker.
* **The engine room:** the drive's ring, its light and hum rising with a flight.
* **Sound by room,** in levels (the synth is mono): the hum loudest in the engine room, the lab's bubbling, the garden's water, the
  dome's quiet, the player's music fuller in the library.

### 2.6 How it is built

* **The ship as rows:** rooms (deck, bounds, floor, ceiling, walls with their windows, arches and lamps), openings (arches, stairs),
  fixtures (furniture as boxes, prisms and cylinders, each with its material and shade) and devices (today's `cabinDevices` rows
  with a room and a place). A mesh is built once in ship metres with base shades; the frame adds the light, the flicker and the
  screens' shades. The walk's collision is the plan's walls and the fixtures' footprints, and stairs give the floor's height by place.
* **Drawing:** the rooms in view through the openings of the room you stand in (portals); each polygon projected once a frame
  before the bands, each band filling only those that reach its rows; the space view drawn only where a window or the dome is in
  view, else skipped and its banks freed.
* **Banks:** looking out, the cabin's thirteen (8, 9, 10, 21-30) are dealt each frame. The hull and the light are fixed; the others
  go to what is in view (screens nearest first, materials, the jars, the drones' paint), and a far screen past the dealing dims to
  its frame's grey. Looking in, all 32 are dealt. Each device keeps its colour, 0.3 apart from every other (R-404's unit check reads
  the table's count, not a literal ten).
* **Cost:** the space 4x row (the start's view; median 6.33 ms, budget 9.5) holds, and new rows (the bridge, the hall, the library,
  the lab and the dome at 4x) are budgeted at 1.5 times their medians.

## 3. How it plugs in

* **`src/life/` (new):** it reads `src/galaxy` and `src/core` and is read by the surface, the game, the harness and the peoples' one
  file of made organisms. Its files: `biota` (the stage, the chemistry, the species: `biotaOf`), `lineages` (the geneses, the kin
  cells, the drifters' origins), `morphs` (the form genome), `morph_draw` (a form drawn in a way of seeing at a time, into a
  framebuffer), `cultures` (the growth model), `sequences` (the genomes, their distances, the tree). It has its own CLAUDE.md, and
  the layering in `tools/check.py`.
* **The surface:** `surface/sampling` (the microhabitat at a point, a sample, the lamp's fluorescence); a trait (red snow) and the
  springs' bands in the planet function, by the Q protocol.
* **The game:** `samples` (the kit, the cold store), `microscope` with `microscope_draw`, `incubator`, `sequencer`, `tree_screen`,
  `lab_state.h`; for the ship, `ship_plan` (the rows), `cabin` (the walk, the draw) and `ship_light`. game.h passes 48 KB
  (PLAN-screens.md section 8 revisits at 60 KB; the user raised the mark to 70 KB on 2026-11-19, KI-445).
* **States and rows:** MICROSCOPE, INCUBATOR, SEQUENCER, TREE and CATALOGUE before `GameState::COUNT`; the devices before
  `FACE_COUNT`; a `TextKind` for a species and one for a strain.
* **The guide:** `M` keys and a format line, `microbes 1`. **The save:** the lab's lines, through a state row's `save`, `load` and
  `reset` (the first rows to use them).
* **The harness:** `life` (list, survey, kin, plates, strips, distinct, search, made), `lab` (microscope, cultures, jars, tree) and
  `ship` (rooms, walk, light); `fingerprint life`; the tour reading the devices' rows; the battery's new items.
* **Generation 17,** from Z-01: the biota is a new part of every body. No draw of 16 moves (salts of its own, `salt scan`).

## 4. Sources

* The limits of life: L. J. Rothschild and R. L. Mancinelli, "Life in extreme environments", Nature 409 (2001); K. Takai et al.,
  growth at 122 C, PNAS 105 (2008); N. C. S. Mykytczuk et al., growth at -15 C (Planococcus halocryophilus), ISME J 7 (2013);
  E. I. Friedmann, "Endolithic microorganisms in the Antarctic cold desert", Science 215 (1982).
* Mats, springs, snow and seas: A. C. Allwood et al., an Early Archaean stromatolite reef, Nature 441 (2006); T. D. Brock,
  Thermophilic Microorganisms and Life at High Temperatures (Springer, 1978); R. W. Hoham and D. Remias, "Snow and glacial algae: a
  review", J. Phycol. 56 (2020); A. Oren, Halophilic Microorganisms and their Environments (Kluwer, 2002); S. D. Miller et al.,
  "Detection of a bioluminescent milky sea from space", PNAS 102 (2005).
* Other chemistries and other worlds: H. Morowitz and C. Sagan, "Life in the clouds of Venus?", Nature 215 (1967); S. Seager et al.,
  "The Venusian lower atmosphere haze as a depot for desiccated microbial life", Astrobiology 21 (2021); W. Bains, "Many chemistries
  could be used to build living systems", Astrobiology 4 (2004); D. Schulze-Makuch and L. N. Irwin, Life in the Universe:
  Expectations and Constraints (Springer, 2008); J. Stevenson, J. Lunine and P. Clancy, "Membrane alternatives in worlds without
  oxygen: creation of an azotosome", Sci. Adv. 1 (2015); H. Sandström and M. Rahm, "Can polarity-inverted membranes self-assemble on
  Titan?", Sci. Adv. 6 (2020); N. Madhusudhan et al., "Habitability and biosignatures of hycean worlds", ApJ 918 (2021); J. H. Waite
  et al., molecular hydrogen in the Enceladus plume, Science 356 (2017); F. Postberg et al., "Macromolecular organic compounds from
  the depths of Enceladus", Nature 558 (2018).
* Before life: T. F. Zhu and J. W. Szostak, "Coupled growth and division of model protocell membranes", JACS 131 (2009); M. M.
  Hanczyc, S. M. Fujikawa and J. W. Szostak, "Experimental models of primitive cellular compartments", Science 302 (2003); K. Altwegg
  et al., glycine and phosphorus at comet 67P, Sci. Adv. 2 (2016); C. Sagan and B. N. Khare, "Tholins", Nature 277 (1979).
* Between worlds: H. J. Melosh, "The rocky road to panspermia", Nature 332 (1988); C. Mileikowsky et al., "Natural transfer of
  viable microbes in space", Icarus 145 (2000); F. C. Adams and D. N. Spergel, "Lithopanspermia in star-forming clusters",
  Astrobiology 5 (2005); P. C. W. Davies et al., "Signatures of a shadow biosphere", Astrobiology 9 (2009); S. Hoshika et al.,
  "Hachimoji DNA and RNA", Science 363 (2019).
* Forms: E. Haeckel, Kunstformen der Natur (1904); J. Gielis, "A generic geometric transformation that unifies a wide range of
  natural and abstract shapes", Am. J. Bot. 90 (2003); K. D. Young, "The selective value of bacterial shape", Microbiol. Mol. Biol.
  Rev. 70 (2006); A. E. Walsby, "A square bacterium", Nature 283 (1980); N. Y. Kiang et al., "Spectral signatures of photosynthesis
  II", Astrobiology 7 (2007); S. DasSarma and E. W. Schwieterman, early purple retinal pigments, Int. J. Astrobiol. 20 (2021);
  D. L. Kirk, Volvox (Cambridge, 1998); D. B. Murphy and M. W. Davidson, Fundamentals of Light Microscopy and Electronic Imaging
  (Wiley-Blackwell, 2012).
* Motion and behaviour: E. M. Purcell, "Life at low Reynolds number", Am. J. Phys. 45 (1977); H. C. Berg and D. A. Brown, chemotaxis
  in E. coli by three-dimensional tracking, Nature 239 (1972); C. Brennen and H. Winet, "Fluid mechanics of propulsion by cilia and
  flagella", Annu. Rev. Fluid Mech. 9 (1977); R. Blakemore, "Magnetotactic bacteria", Science 190 (1975); K. H. Nealson and J. W.
  Hastings, "Bacterial bioluminescence", Microbiol. Rev. 43 (1979); K. J. Tomchik and P. N. Devreotes, cAMP waves in Dictyostelium,
  Science 212 (1981); J. M. Greenberg and S. P. Hastings, excitable media, SIAM J. Appl. Math. 34 (1978); E. Ben-Jacob et al.,
  "Generic modelling of cooperative growth patterns in bacterial colonies", Nature 368 (1994).
* The lab: J. Monod, "The growth of bacterial cultures", Annu. Rev. Microbiol. 3 (1949); G. F. Gause, The Struggle for Existence
  (1934); R. E. Lenski and M. Travisano, a 10,000-generation experiment, PNAS 91 (1994); Z. D. Blount, C. Z. Borland and R. E.
  Lenski, citrate in E. coli, PNAS 105 (2008); E. A. Rundell et al., 16S rRNA of Winogradsky columns, PLoS ONE 9 (2014); R. J. Cano
  and M. K. Borucki, spores in 25-40-million-year-old amber, Science 268 (1995); R. H. Vreeland, W. D. Rosenzweig and D. W. Powers, a
  250-million-year-old bacterium from salt, Nature 407 (2000, disputed); A. Shatilovich et al., a nematode revived after 46,000 years
  in permafrost, PLoS Genet. 19 (2023); D. G. Gibson et al., "Creation of a bacterial cell controlled by a chemically synthesized
  genome", Science 329 (2010).
