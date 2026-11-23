# The living small

Microorganisms on every world (R-425, 1.5.0's Z series; the design `docs/ideas/IDEAS-small-and-spacious.md` section 1). `src/life/`
reads `src/core` and `src/galaxy` alone. Z-01 made the biota (generation 17), Z-02 its forms and the slide they are seen on, Z-03 the
search (the samples, the kit, the ultraviolet, red snow and the springs' bands), Z-04 the microscope (a vial's slide, what is done to it,
what is seen of each species), Z-05 the incubator (cultures in twelve flasks, their stages, spores woken, two in a flask, strains), Z-06 the
genomes, the sequencer, the tree of kin and the organisms the late peoples made, Z-07 the record (the catalogue, the LIFE
page, lending, auto-explore's samples). Then, on the user's word: R-432 the microscope's lists by world and the cold store without limit,
R-433 the culture collection, R-434 the activity vision; the Y series (R-443, R-444): Y-01 the trees by world and by folder, Y-02 the genes
and the gene editor, Y-03 genes from others, Y-04 the switches; R-447 took the jars out. A culture sent down to a world changes it (R-448,
the J series): `14-seeded-worlds.md`.

<!-- contents -->
Contents:
- What a world holds (`life/biota.h`)
  - The rows (`BIOTA`)
  - The stage
  - The habitats
  - The species (`Microbe`)
- Whence it came (`life/lineages.h`)
  - Kin cells
  - Drifters and wanderers
  - Carried spores
- What it looks like (`life/morphs.h`)
  - The values
  - Kept apart
- The slide (`life/morph_draw.h`)
  - Six ways of seeing
  - Drawing a specimen
  - The acts
- The search (`life/samples.h`, `surface/sampling.cpp`)
- The microscope (`life/slides.h`, `game/microscope.cpp`)
- The incubator (`life/cultures.h`, `game/incubator.cpp`)
- The genomes and the tree (`life/sequences.h`, `game/sequencer.cpp`, `game/tree_screen.cpp`, `peoples/made.h`)
- The genes (`life/genes.h`, `game/editor.cpp`)
- The seeded worlds
- The record (`game/catalogue.cpp`, the guide)
- Determinism, cost and the cache
- The harness
<!-- /contents -->

## What a world holds (`life/biota.h`)

A `Biota` is a landable world's: its `stage` (the stage its life reached), a `second` it shows (FOSSIL in its old rock, PREBIOTIC
under carried spores), its `chemistry`, its `genesis`, its `habitats` (the microhabitats it has, `Niche` bits), `dormantYears`
(how long a dormant world's life has slept), `meanC` (its ground's mean, as the limits read it: `biotaMeanC`, the living climate's
B-321 curve or an air's or an airless world's rule, the dark's own heat) and its `species`. A giant, a brown dwarf and the companion
have none (X-03: no life in a giant's clouds).

### The rows (`BIOTA`)

A row a kind, built at compile time (`biotaOk`: a row a kind in order, a furnace sterile and keeping nothing, a row with habitats a
stage of life or the spores and a chemistry). The unit holds it against `KINDS` (every landable kind with habitats, every other
none; a world with flora colonial; a microbial kind small cells; a dead kind dormant over fossils). A row has:

- `w`: the weights of the stages its own life reaches (STERILE to DORMANT), `fossils`: the share whose old rock keeps fossils;
- `chemistry`: WATER, ACID (the venusian and acidic worlds' cloud droplets), AMMONIA, HYDROCARBON (the methane lakes);
- `furnace`: nothing lives and nothing comes (molten, volcanic, tectonic, magma ocean, chthonian, glass, steam);
- `keepsSpores`: airless or icy, it keeps a living neighbour's spores; `underIce`: its life is in a sea under its ice (europan,
  brine dwarf, grooved ice, tiger-striped), so the surface's cold does not end it;
- `habitats`: its microhabitats.

The stages by kind follow IDEAS 1.1: the living kinds with flora colonial; the ocean and eyeball worlds large cells or colonies; the
archean small cells (one in five large); the lichen world large cells; the venusian and acidic worlds' clouds, the thin airs, the
quartz worlds' hypoliths, the salt, rust, hycean, ammonia, snowball, chemotroph and dark-ocean worlds small cells by their weights;
the dead felisian worlds and the rust world spores over fossils; comets, the hydrocarbon worlds (one in six small cells) and the far
cold's tholins prebiotic; the airless rock and ice sterile but for carried spores.

### The stage

`drawWorld` picks it among the row's weights by `SALT_STAGE`, as the world's facts allow: in the dark (a drifter's system) the active
stages become DORMANT but the dark ocean's and the seas under ice; elsewhere the active stages need the chemistry's liquid at the
world's mean (`activeAt`: water -60 to 110 C, ammonia -110 to -10, methane -205 to -150; the clouds always) unless its life is seen
on its ground (flora, fauna, mats, vents); a glassed world's own life none but where it shows on its ground (a chemotroph's vents
outlived the glassing); a people's world at least large cells. A drifter's airless and icy bodies keep the spores of the star it
left one in three (`SALT_DARK`).

### The habitats

The row's, then the traits' (hot springs and geysers a SPRING; pink lakes, salt flats and gypsum SALT; a people's RUINS), a sea
frozen in the dark ICE, a glassed ground without soil, mats or crusts. The species are drawn in the livable ones: the niches with a
warmth of their own (a spring, a vent, ice, snow, a plume, the clouds) and the ground where the mean lets the liquid be (none on a
venusian's burning ground); a sea under ice's in its ice, plumes and brine. The first species take each livable habitat once in a
hashed order (`SALT_ORDER`; the ruins' life comes from the soil and the rock), the rest by weight.

### The species (`Microbe`)

A world of life has 3-16 living species (small cells 3-8, large 5-12, colonies 8-16), a dead world 1-4 spores, a prebiotic world
2-4 kinds of protocell (no genome, no genesis), a living or dead world with fossils 2-4 fossils. Each draws from its own stream
(`SALT_SPECIES` and its index): its niche and the niches it also lives in (`ALSO`), its way (light, chemistry, dead matter,
grazing, preying: the large cells' alone; no light in the dark or under ice), its plan (`MicrobePlan`: the protocells; the small
cells' eleven shapes; the large cells' eight; the colonies' five), what it burns (`Substrate`), its pigment (by its sun's class:
Kiang 2007's darker light under a red dwarf; the halophiles' purple and red, the snow algae's red, the mats' orange), the oxygen it
bears, its temperatures (round the niche's warmth: a spring's band 45-95 C, a vent 65-110, the ice's psychrophiles, the clouds'
temperate height), salt, pH, the dose it survives, its life cycle (`LifeCycle` bits: spores and cysts, exchange, colonies,
gathering and fruiting), its division, its motion and taxis (a field only where the world has one), a glow, its share, its clade
and its seed (its form's, Z-02). Then fixes by hashes: a living world has a spore former; the milky seas and the lights in a dark
sea a glowing species, the vent world's glowing spores, the luminous forests' foxfire.

The ship's three (`shipSpecies`): the garden's soil rod, the explorer's skin's sphere, the garden pond's green filament, of home's
genesis.

## Whence it came (`life/lineages.h`)

A `Genesis` is an origin of life: its `id`, its chemistry, its origin (OWN, CELL, WANDERER), whether its handedness is home's mirror,
its letters (4, 6 or 8) and style, its age. Its traits come from its id (`SALT_TRAITS`).

### Kin cells

Cells of `KIN_CELL` (64) sectors a side hold one or two old geneses (`cellGenesisCount`, two in 0.4); the first is the oldest (6-8.5
billion years). A system's water life is its cell's (one of them) one time in five (`systemGenesis`, `SALT_KIN`, `KIN_SHARE`), else
its own; every other chemistry its own. So a system's living worlds of one chemistry share a genesis. Home's is pinned: Heilya's
water life is its cell's oldest (cell 3 0 0, #0), ours in hand, of four letters (`HOME_GENESIS_ID`, the salts KIN and TRAITS chosen
for it); within 200 ly of home 47 systems share it.

### Drifters and wanderers

A drifter's life is the star's it left (`drifterPast`'s, its genesis by that star; its own where no star was found), asleep for the
years since it left (`drifterLeftYears`). A wanderer (P-05's caught rogue) has a genesis of its own (`wandererGenesis`), which no
star near it shares.

### Carried spores

After every world is drawn (`makeLife`), impacts carry a living world's spores to its neighbours with no life of their own that keep
them (airless or icy, no furnace, no wanderer): two bodies are neighbours when of one family (a world and its moons) or when no world
of their sun's order lies between them. A neighbour takes them from a living world of its family (0.85) or else from the nearest
beside it (0.45), one or two of its spore formers, preferring those that outlast the time since their impact (`SALT_CARRIED`). A
carried spore is its source's species (its seed and plan), DORMANT, in the neighbour's ice, dust or rock, with `fromBody` and
`fromIndex`; the neighbour takes the source's genesis and chemistry.

## What it looks like (`life/morphs.h`)

A `Morph` is a species' form: about forty values in groups, drawn from its seed by its plan, its state, its world's chemistry and
the style its genesis gives its kin. Nothing of it is saved; the drawing reads it (`life/morph_draw.h`).

### The values

- The outline: Gielis's superformula (a symmetry and three exponents: Gielis 2003), length over width, a rod's bend, a helix's
  amplitude and turns, how far the outline wanders, the ends' taper (pointed, round, square).
- The size (0.12 um a spirochete's width to 2 mm a foram: Young 2006) and the arrangement (single, pairs, chains, clusters, fours,
  packets, palisades, rosettes; a colony's count, a filament's cells).
- The wall: its thickness, its ornament (`WallMark`: pores, ribs, spines, a lattice, plates, a net, the world's grains), spines by
  count, length and branching, a capsule's halo, a sheath.
- The appendages: flagella (count, place, length, beat, wave), cilia (cover, rows, length, the metachronal wave), pili, a stalk and
  its holdfast, pseudopods (lobes, threads, a net).
- The inside: a nucleus (a nucleoid in the small), vacuoles (contractile, pulsing; food, with their prey), granules, plastids
  (discs, a cup, a band, a star, a net) and pyrenoids, an eyespot, gas vesicles, a chain of magnetosomes, droplets (oil, sulphur).
- The pigment and its density; the motion's speed and runs; the forms of its life cycle (a spore's coat smooth, spined, ridged
  or netted, its size and place: central, under a pole, at a pole).
- The marks the eye tells small cells apart by: refractile granules at a pole or both, the nucleoid's clear zone, a wavy axis; a
  protocell's organic tint (tholins in the methane: Sagan and Khare 1979), its clay grains (Hanczyc et al. 2003).

Each plan has its variants (`variantsOf`): a diatom centric, pennate, bow-shaped or polygonal; a test an urn of grains, a dome or
scales; a lattice a sphere, twenty spines from the centre or a helmet; chambers on a spiral, in a line or a trochospire; an armoured
one rounded, horned (Ceratium) or winged; a flagellate euglenoid, two-flagellate, many-flagellate or collared; a ciliate a slipper,
a trumpet, a bell on a stalk that coils or a walker on cirri; an amoeba lobose, filose, a heliozoan or a slug; colonies Volvox and
Pandorina, Gonium, Pediastrum and Merismopedia, Scenedesmus, Chaetoceros, zigzags and beads in jelly, mycelia and branching algae.
`everyForm` gives every plan's every variant (75).

The style (a genesis's 32 bits): a symmetry it favours (3-8), its walls' mineral (silica, calcite, celestine, iron, organic; sulphur
in the acid, a nitrile membrane in the methane), its flagella's kind (a whip, a tinsel of hairs, a ribbon), how angular, ornate and
spiny its forms run, and its hand: a mirrored genesis's helices, coils and spirals turn the other way. Kin share it; a stranger's
differs.

### Kept apart

Each group draws from its own stream (`SALT_MORPH` and the group), so a value added at a group's end moves no other. `morphsOf`
gives a world's forms in its list's order: a carried spore is its source's form, as drawn on its world; the world's own are drawn
again (a nudge: the best of 24 draws) until none is alike one kept. `morphDistance` weighs the strongest values as A-04's genome
distance does (categories 0 or 1, measures by their spread, colours by distance): 20.4 at most; under `MORPH_ALIKE` (2.5) two look
alike. The threshold is the pictures' evidence: pairs of forms within 2.5 look alike in pictures 8% of the time, thirteen times as
often as two at random (0.61%), and 0.46% of random pairs fall within it. Over the 300 worlds with species nearest home no two of
a world are alike (7,649 pairs, the nearest 2.52); of 1,000 species' 96-px pictures in a dark field 0.61% of pairs look alike
(C-17's measure, a mean difference under 6; in phase contrast 3.7%, in a bright field 4.9%, where thin and faint forms leave the
picture mostly field).

## The slide (`life/morph_draw.h`)

A `Slide` is a view (its centre, micrometres a 1x pixel, its way of seeing, the clock, the lamp, the electron's turn and tilt, the
section's stage, a field's direction, a spot of light); a `Specimen` a form doing an act from a time at a place, with its own seed
(the individuals differ). `drawSlide` clears a framebuffer and draws every specimen in order; the mush is the caller's (two passes,
as the scenes'). Everything is closed-form in the clock: a frame replayed is the frame seen.

### Six ways of seeing

`slidePalette` sets the 32 banks' ramps for a way (Murphy and Davidson 2012):

| Way | The field | What shows | Banks |
| --- | --- | --- | --- |
| BRIGHT | the lamp's warm white (shade 56) | pigments in their colours, absorbance by thickness; colourless cells faint | 26: field, 7 pigments, eyespot, oil, sulphur, 7 minerals, 3 grains, carbon |
| PHASE | grey (34) | cells dark in a bright halo, their insides clear, spores and droplets phase-bright | the same, tinted grey |
| DARK | black (3) | edges, flagella, granules and the gathering's waves bright | the same, toward white |
| GLOW | black | the DNA stain's blue, the plastids' red, a species' own light, the walls' faint green | 5 |
| ELECTRON | a filter's grey with its pores | one cell's mesh lit from one side, its edges bright, pores pits, plates ridged | 1 |
| SECTION | crossed polarisers | chert, calcite, mica and pyrite in their interference colours and extinction; fossils' carbon | 10 |

A bank's ramp meets the field's at the field's shade (the mush averages across banks): the unit holds it. `seeingBanks` lists a
way's banks; `seeingsFor` the ways that suit a form (a coloured cell a bright field, a colourless one phase; a flagellated small
cell the dark; a walled one the electron; a glowing one fluorescence; spores phase and the electron; fossils the section).

### Drawing a specimen

`buildShape` makes what a specimen is at a time (`life/morph_shape.h`): bodies (a radial outline, the superformula's table with
the wander's harmonics and the pseudopods' lobes, tapered and bent; or a tube along a centreline), dots (the insides), strands
(flagella as travelling waves, cilia in metachronal waves, pili, spines, stalks, threads, bridges) and waves. The light ways draw
each pixel from them: a body's signed distance gives its thickness (relief from its edge), its wall and its ornament (3D on its
dome, both faces of a clear one; or the flat layouts: a centric diatom's radial rows, a pennate one's striae across its raphe,
stripes, helical strips); the parts add absorbance, path, scatter and fluorescence by kind. Anything thin is sized in screen pixels
(a strand `g_mushKernel + 1` wide, a dot that radius) and fades as it widens. The electron's view meshes the bodies (a radial
outline drawn in toward its centre by latitude, a tube swept and capped) and the strands, lit per vertex (the detector's side, the
edges bright), on a filter; a colony's jelly is left out and its cells sit on its sphere. The section is a mosaic of grains, each
its retardation and extinction against the stage, inclusions and strained extinction within, the far mean where grains are small;
a fossil is its outline in carbon, broken, over a paler fill.

`magnificationFor` chooses among the objectives (x20-x1000) and the camera's zoom (to x4000; the electron x50-x40,000) the one that
holds a form's first look (`viewUm`: its body; a filament's few cells, a spirochete's few waves, a mycelium's few branches).
`specimenPlace` is where a specimen is (the microscope's tracking), `specimenBox` the box its parts cover, `specimenExtentUm` the
radius every pixel it draws stays within (the unit's bounds check).

### The acts

Each act (`Act`) is closed-form in the clock from its start, over its time (`actSeconds`), for the forms it suits (`actSuits`):

- life: the motion (runs and tumbles between hashed waypoints, Berg and Brown 1972; the helical swim's meander; gliding and its
  reversals; twitching steps; crawling; rolling; a corkscrew's back and forth; the still ones' Brownian jiggle) and the beating,
  the contractile vacuole's pulse, the granules' streaming, a bell's stalk that coils at once and stretches slowly;
- division (30-60 s: a rod lengthening and pinching in two, a sphere's halves, a large cell across or along its axis, many at once
  inside a wall that gapes, a filament breaking at a dead cell), budding (40-70 s), a Volvox's daughters let out (60 s);
- sporulation (50-70 s: a forespore at its place, bright as its coat forms, the mother lysing), waking (20-60 s: swelling,
  darkening, the coat split, the new cell out), the cyst (30 s), lysis (12 s);
- engulfing (25 s: two arms round the prey, a food vacuole shrinking), the bridge (40 s), the turns to a spot of light (20 s) and
  to the field and back when it is reversed (12 s), the glow switched on as the crowd grows (quorum: 30 s);
- the gathering (120 s: amoebae streaming along spiral waves to a mound, the waves bright in a dark field: Gross et al. 1976), the
  fruiting (a slug, then a stalk rising with its head of spores), a colony's branching front on a plate (Ben-Jacob et al. 1994), a
  protocell's thread grown and split into vesicles (Zhu and Szostak 2009).

## The search (`life/samples.h`, `surface/sampling.cpp`)

Z-03: microbes are not seen; the explorer chooses where life would be and takes a sample. `SurfaceView::readSample` reads the
microhabitat at a point (the crosshair within 2 m of the feet, a swimmer's water) from what the world fixes there: the planet
function at the square metre (its material, its water and its kind, the sea's level, red snow's `snowTint`, the heights a metre
and a half either way for the shade of a slope turned to the pole or a cliff's foot), the springs' finder (`springPartAt`: the vent's
pool, the sinter, a terrace's pool by its band, the runoff), the chemotroph's hot pools, the geysers' sinter, the plumes' and the
curtains' fall, and a dead people's walls; never what the landing deals (the rocks, the domes, the trees). Its niche, its own
mean temperature the year round (`placeMeanC`: the site's formula without the season and the day, a seeded world's made warmth with it
where there is air; a spring's band, a pool's, a
vent's own; the snow and the ice their summer's melt; where life is seen on the ground its habitat at most 50 C), its salt, its
shade, whether its rock is old and whether its life is seen crowded there (a mat, red snow, a spring's band: ten to thirty times its
niche's crowd), the temperature read at the take, a name ("WET SILT AT A LAKE'S EDGE", "SINTER AT A SPRING'S RIM", "RED SNOW",
"OLD WALL"); mats no species of the world lives in are its springs' life. `placeKey` is the square metre (the body-fixed point
rounded to the metre on its radius, with the body's seed).

`sampleOf(biota, microhabitat)` is what a vial holds: each species whose niches hold the place's, by its share (a common one in
most square metres), its count its niche's crowd (log10 cells a millilitre: a mat's 9, silt and soil 8.5, the open water 6, the
snow 4.3, the dust 2) by its share and a decade either way by the place; a salt it bears but does not grow in 100 times fewer, a
light-eater in shade 30 times; out of its range it waits there, inactive; spores few; fossils in old rock a few a section;
protocells where their chemistry gathers. The activity is the labelled release's reading (Levin and Straat 1976): STRONG for a crowd
at work (10^5.5 a millilitre), FAINT for a few or a protocell's chemistry, NONE for spores and fossils (it can mislead). A sterile
world's samples hold nothing alive (a furnace's at most cooked organics).

The ultraviolet (Shift+U on foot) lights the living films within 18 m in a cone of 24 degrees: each near corner's niche and share
(`filmNicheOf`: a mat 1, red snow 1, a shore's line 0.8, crusts 0.8, the soil 0.35, the rock's endoliths 0.12) by the niche's
density (`filmDensity`: its living species' shares), patchy in the world's half metre, its colour by the pigment most of the
world's cells carry (`filmGlowColour`: chlorophyll's red, the carotenoids' orange, a chemotroph's blue-green). It is drawn after
the old result: its light added to the pixel's, in bank 15 where it is the brighter (the spores', the milky sea's or the lamps'
ramp on worlds that have one); off, the pass and the ramp touch nothing.

The activity vision (R-434, Shift+O, on foot or from a vehicle's camera) shows where a vial would read activity. Each square metre
within 40 m is read at its middle as a vial reads it (`readSample`, then `sampleOf`'s crowd at work, `Sample::activeLog`), the nearest
first, 384 a frame on the pool (the ruins' walls after on the main thread, `wallsDone`): a quarter of a second fills its reach, and the
metres are kept for the landing (the tide past a quarter metre reads them again). Its pass is the frame's last: every pixel into bank 15,
the vision's ramp while it is on, a thermal camera's way: the ground's and the water's own pixels within reach by their metres' heat
(blended between their middles), none at work dark but for the ground's shading, faint red to orange, strong yellow to white by its
crowd (a mat or wet silt whiter than the soil, the soil than the open water); everything else (the sky, a stone, a trunk, the buggy,
the ground past the reach) dimmed by its luminance. Off, the pass and the ramp touch nothing; the ultraviolet and it one at a time.
## The microscope (`life/slides.h`, `game/microscope.cpp`)

Z-04: the laboratory's first device, a camera entered from the stern bench (`E`; the state `MICROSCOPE`, its row teal). Its first
page is its lists (R-432, `game/lab_lists.cpp`): THE VIALS, a folder a world in the cold store's order, each world's vials with the
activity each read (Enter puts one on the stage; Backspace empties it), and THE CULTURES (R-433): the flasks' cultures and the
collection by folder. The cold store holds any number of vials (its door's lamps by doublings, `drawStore`); the slide's page shows the
vial's mount. Nothing of a slide is saved: it is made again from the vial.

**The mount** (`mountOf(biota, forms, sample, microhabitat)`, a pure function of them): a drop under its cover glass (20 um deep), a
rock's sample a thin section of it (30 um). It is prepared as a microbiologist would, concentrated or diluted by tens (`prep`, -6 to
+6, the HUD's `DILUTED 1:100` or `CONCENTRATED X10`) so that its most crowded species covers about a sixteenth of the glass (a body
over 20 um counts as 20: it is diluted for the cells, not the filaments); no species covers more than an eighth (the large ones a few
in a field). Each held species is a kind: its form as found (a cell; a spore, or a large cell's cyst, for the dormant; a fossil),
whether it lives and grows at the stage's warmth (the vial's: the stage keeps it), what it turns toward and its speed, the warmth that
stresses it and its answer (`AN_SPORULATE` to a spore, `AN_ENCYST` to a cyst, else `AN_LYSE` to a ghost). Its cells, a hundred of its
bodies' tile at its density (160 at most, a cell at least, the tile 1.5 mm at most: a rare species over-represented to be found), repeat
across the slide at the tile's period, each copy its own seed, heading and depth (`SlideRef`: a cell and its tile). Their depths: the
swimmers through the drop (rising and falling a few micrometres), the gas-filled under the cover, the rest settled on the glass, a
cell thicker than the drop pressed under the cover. Some of a colony former's are found in a microcolony, a few dividing. The first
look: the most crowded kind's first cell (a section's first fossil), its way (`seeingsFor`, never the electron's), the magnification
that holds its body in a sixth of the view's height, its depth in focus.

**The events** (`SlideEvent`, on the slide's clock): `L` a spot of light at the crosshair (named LIGHT on the HUD, the field brighter in it;
`L` again takes it away), `M` the magnet beside the stage, turned half round, taken away (R-436: drawn at the view's left edge, its pole
facing the slide lit, its field's lines across the view four seconds after each press), `H` the stage's warmth typed (R-435: within the
drop's liquid range, its chemistry's cell's; Shift+H the vial's again; eased to it, 12 s), `F` a drop of broth at the crosshair (named
FOOD, its scent's front spreading as a fainter ring, `slideScentUm`; `F` again rinses the slide). `slideStateAt` folds them;
`mountSpecimens` gives the specimens in a view at a time, closed-form. R-438: a kind lives on the slide where its drop suits it (`lives`:
a culture's verdict, `Held::alive`, else the vial's salt) and moves while the stage's warmth is in its range, at its pace there (its
cardinal curve's, `cardinalRate`, against the vial's warmth's: its species' speed is its speed there; 0.2 to 2), by its own clock
(`PaceClock`: the slide's till the stage is set, then a table through each easing; `Specimen::clock` carries it to the pose). Every copy of
a kind that turns to light or food (`TX_LIGHT`, `TX_FOOD`), within its reach (four radii of the spot, or what it swims in 25 s, to twelve),
runs to a point of the crowd it joins (packed to twice the spot's radius) as it senses it: the light at once, the food as its scent's front
reaches it (a diffusion of 1,000 um2/s); its wander narrowed as it goes (`Specimen::wander`, its heading turned by its drift `vx`, `vy`). A
swimmer of the dark (of soil, silt, a vent, rock or a plume, no light-feeder) inside a spot swims out past its edge; an eater beside a
spot on living light-feeders is drawn to them from ten seconds on (Engelmann 1882). A magnetotactic one answers the field alone: it swims
along it from the magnet's coming, a U-turn of 1.6 s at each turn (its side alternating, so its sideways steps cancel), let go when the
magnet is taken away. A kind whose warmth passes its top (+2 C) going up, or is set warmer still over it, does its act from where it is
(2-12 s apart), moving or still, and ends still in its form. A spore, a cyst, a cell out of its range of warmth, a microcolony are still
(`Specimen::still`: the jiggle alone, the beat stopped). The depth's blur (`Specimen::blurUm`: past half the depth of field,
`depthOfField(mag)`: 25 um at x40, about one at x1,000) spreads a cell's edges and fades it; a copy whose own extent (its wander placed)
misses the view is left out; past 3,000 of a kind in a view an even share of its copies is drawn. Under the electron the slide's clock
stops (a fixed sample).

**What it answered** (R-437, `slideAnswer`: a kind's bits at a time): what each thing done is for is said as it is done ("A SPOT OF LIGHT
- A LIGHT-FEEDER SWIMS INTO IT"); each kind in view is told once a slide as it answers (into the light, out of it, to the food, round
the lit cells, along the field; moving at a warmth, stopped in the cold or the heat, its stress's act), its state as found not said, a
change of it said; an experiment no kind in view answered in its while (15 s; the food's while its scent crossing half the view, to 45 s;
the warmth's once the stage settles) is said ("NOTHING IN VIEW ANSWERS THE LIGHT"; the magnet's with "MAGNETOTACTIC CELLS LIVE IN SILT AND
MUD"), the moving kinds in view not answering it kept so. The lines wait their turn (`MicroscopeState::said`), four seconds each.

**The view** (`MicroscopeState`, `lab_state.h`): 1-6 the ways (V the next; SECTION a section's only), `+`/`-`, the wheel or Page
Up/Down the zoom (x40, x100, x200, x400, x1,000, x2,000, x4,000; the electron x200 to x40,000), its shown magnification eased; `[` and
`]` the focus (a press half the depth of field, held a steady turn); the arrows and the mouse the stage (the electron's mouse turns
and tilts its cell); Enter tracks the cell under the crosshair (the stage and the focus follow its copy, `mountSpecimen`; the mouse
rests meanwhile, the arrows let it go). The slide is
drawn with `Slide::lod` (a cell under 2.5 of the view's 1x pixels a speck: its body's size and look, `speckOf`) and, at 3x and 4x,
`Slide::step` 2 (the light ways evaluate a pixel in four, each its 2 x 2 block's, under a mush that spans more; the electron's mesh
half as fine); the electron builds its bodies' meshes on the pool and draws them in row bands, each mesh row skipped where it misses a
band. Off the microscope, every new field's default draws as Z-02's plates (the battery's `life_look`).

**The record:** what is seen of a species (a cell in the view's middle, in focus, 5 of the view's 1x pixels across) is kept in the
guide by its key `<body>/M<index>` (`MicrobeSheet`: its form on any slide and the forms of its life seen; on a cell, its motion on a
living slide, its flagella in a dark field at 8 pixels, its DNA in the glow at 6, its wall under the electron at 24; `microbe` lines
after `microbes 1`). A form's first sight is said (`A FORM NOT SEEN BEFORE: A CURVED ROD, 2.4 UM BY 0.6 UM`) and logged (`FORM`).
Tab shows the sheet of the species under the crosshair: its name (UNNAMED, or the explorer's: `N`, a text entry of `TEXT_SPECIES`) and
world (or UNKNOWN), then only what was seen (a spore's coat, a fossil's outline, or the cell's form, colour, motion, flagella, DNA and
wall, the forms it was seen as; R-437, what it answered: TO LIGHT, TO FOOD, TO THE FIELD, or NO ANSWER SEEN, and WARMTH, the stage's
warmths it was seen moving and still at and its stress's, the guide's `answers` line after the sheets) and the ways that would show more.
`P` takes a micrograph to the gallery (the photograph's key, its caption the way, the magnification, the species and the vial).

## The incubator (`life/cultures.h`, `game/incubator.cpp`)

Z-05: the laboratory's second device, a screen over the room (`E` at its cabinet on the stern bench; the state `INCUBATOR`, its row
amber, home 20). Twelve flasks behind its glass door, three shelves of four, each a medium and a culture of one or two species. All
of it runs on the lab's clock (`LabState::clock`, the drones' kind: real seconds while the game
simulates, saved; none in the menus).

**A flask** (`Flask`, `lab_state.h`): its species (`FlaskSpecies`: a vial's world and the species' index there, its strain now and the one
it went in as), its medium (`Medium`: its cell, water's or the acid cell, the ammonia cell, the cryostat, each its chemistry and its range;
its warmth; its light, dark, white, red, blue or ultraviolet (J-01: UV); its energy, a substrate or the broth; its salt), its events on
the lab's clock
(`CultureEvent`: started, the medium changed, a fresh broth, a second added, a strain) and AS ITS WORLD (`mediumOfPlace`: the vial's
chemistry's cell, the warmth read at the take, the sun's light where it reached the place, the place's energy, a spring's sulphide, a
plume's hydrogen, a rust world's iron, else the broth; its salt), the first guess a species goes in at. Enter puts in a species seen in a
vial (only what the microscope showed of it: never a fossil or a protocell), a flask's culture or a kept one; on a culture a second. The
chooser (R-432's folders) has two tabs (R-441): THE VIALS, the raw samples by world, and THE CULTURES, the flasks and the collection's
folders; Tab between them, the chooser opening on the one last shown (the sequencer's alike, each species once among the vials and once
in each of the cultures' folders). A species with no name is its look in every row (B-442, `flaskSpeciesWords`): a flask "A ROD + A
SPHERE" (its first and a plus where both do not fit), a chooser's row sized ("A ROD, 2.5 UM BY 1.2 UM - FROM ACID FROM A LAKE"), two
rows of a folder alike its forms' colour and motion, then a number (`apartRows`). N names a flask's organism (R-442): of one at once, of
two (or a strain) the one chosen first; the name is the guide's, the microscope's.

**A culture** is closed-form from its events (`runCulture` lays its laws out as segments, `cultureAt` reads one): a lag (Baranyi and
Roberts 1994), the logistic rise at the medium's rate (the species' cardinal temperatures, Rosso et al. 1993; its energy; a light-feeder's
light by its pigment; a salt one step out its range's edge), its crowd by its energy's yield and its family (a small cell's 10^9, a large
cell's 10^6, a colony's 10^5), and held there for good while its medium suits it (R-449: the incubator feeds it, `AT ITS CROWD`, shown
as FULL). With nothing it eats it holds a few minutes (`STARVING`; its hold's end exact, B-444) and then declines: a spore former
sporulates, a large cell encysts, the gathered amoebae gather and fruit, the rest die (lysed: the drop's ghosts). Its lag's work is its
species' (Baranyi's h0 at its best rate before its genes' price, Y-04: a priced line's lag longer). Too hot, the wrong salt or solvent kill
at once (a spore former's spores among them); too cold it waits. A species' pace comes from its seed (R-449: its crowd at its best in 2 to
5 minutes, a quarter of Z-05's 8 to 20; its lag 10-25 s, its spores' waking 5-15 s, its hold with nothing to eat). Spores wake once the
medium suits them and feeds them, most of them, the rest asleep (`WOKEN AFTER ABOUT 12,000 YEARS`: its world's end, its drifter's leaving
or the impact that carried it); a luminous crowd glows (from 3% of its crowd: Vibrio's quorum). The stages a curve passes are each species'
own (`CultureStage`). A line's world genes (J-01, `CultureWorld`) move its crowd (STOP); one that never stops passes its crowd by two
decades and stays OVERGROWN till the flask is emptied. Below its salt's freezing point (fresh 0 C, the sea's -2, brine -20) a water flask
stays liquid unless a culture's ice forms at its warmth (FROST): then everything in it waits, FROZEN. The UV light kills a line without a
shield (KILLED BY UV), and feeds a shielded light-feeder little.

**Two in a flask** (`pairOf`): mirrored hands ignore each other (each its own crowd); a grazer or a predator and a smaller cell cycle (the
prey alone till the predator reaches its measure, then a damped Lotka-Volterra turn about its balance, the prey a decade down; the turn
2 pi over the root of the prey's rate and the predator's death); a light-feeder and a heterotroph, or a fermenter and a hydrogen-eater,
feed each other (the fed one held to a share of its feeder); the rest compete (Gause 1934: the one at its crowd, else the quicker, wins;
the loser falls once the winner holds half its crowd). J-01: a poison one makes and the other does not kills the other, POISONED from
when the poisoner holds a hundredth of its crowd (an e-fold in half a minute at the poison's strongest). The screen says the pair once its
curves show it. In a predator cycle, a switched
strain's state follows its numbers: it counts as crowded while past its quorum share, and the cycle is split into parts where that
changes, each part growing at the rate of its state (`cycleCross`, B-446). A strain that cannot grow in its new state breaks the cycle.

**Strains:** a culture growing at its range's edge (its warmth past half its rate, a salt one step out, a light its pigment takes poorly)
counts its generations; at twelve a strain arises (`STRAIN_GENERATIONS`, Lenski and Travisano 1994): its highest warmth up, its lowest
down or its cells a quarter bigger (Atkinson 1994), its salt taken in, or its pigment the light's. The clock passing its time appends the
event (at that time) and the guide's `strain` line (`<body>/M<index>/S<n>`, its parent and its move); N names it (`TEXT_STRAIN`: the
strain's row among N's).

**The screen:** the flasks (each its species, in its stage's colour), the chosen one's curve on a log scale since it was started (a
pair's two, spores dotted), its species' stages and pace, its pair, what it came to (woken, a strain, the glow), its medium (the field
chosen lit: Left and Right, `+`/`-`, Shift ten degrees; R-435, its warmth typed: a digit begins it, Enter sets it as one change within
its cell's range, Esc keeps it; W as its world again; F a hundredth into fresh broth, a serial transfer; Backspace empties it); N names,
K keeps a copy (the collection). The microscope's list holds the flasks after the vials: Enter puts a drop of the culture
as it is on the stage (its cells, a quarter of them dividing while it grows, its spores, its lysed, the gatherers' slugs and stalks).
What the curve shows of a species goes to its sheet (`MicrobeSheet::stages`, its quickest doubling seen and the medium of it: the sheet's
`STAGES SEEN` and `GROWS` lines). In the room the flasks' liquids take their culture's tone (its pigment's, a broth's cloud), lit
through by their light,
aglow with a crowd's own; their tones are homes past the banks (`ShipBanks`' 32 to 62), dealt a free bank while in view.

**What a flask shows of its world genes** (J-01, `flaskShow`): bubbles of its gas while its cells grow (fewer at its crowd), its water
dyed (the dye stays after its cells die), a scum of floating cells, slime, a sediment of its shells (chalk, glass, rust, sulphur), its cells
in clumps, overgrown (tissue where its cells bind, else packed cells), ice, the other poisoned, genes passed to the other (two of one
genesis, once both grew). The screen says them under the species' lines and draws the chosen flask under the list (its liquid, scum,
sediment, clumps, rising bubbles, its lamp's light about it); in the room the flask's liquid takes its dye, tissue (up its neck) or ice
(the glass's own tone, needing no bank), its sediment under it and a scum on top. A wild species shows its own (a methanogen's methane, a
red snow alga's red); a long name or stage is cut to fit, never left out.

There are no jars (R-447): a flask shows what a species lives on, and the sheet's GROWS line keeps the medium it grew best in. An
older save's `jar` lines and an older guide's `jarband` lines are dropped when loaded.

**The collection** (R-433, `game/lab_lists.cpp`): every species and strain a flask grows, kept frozen as labs keep their stocks. A
species is kept once its curve passes its lag (`cultureStagesBy`'s GROWING), a strain as it arises (`keepCulture` from `updateLab`;
the flask's `FT_KEPT` bits say so), each once, with the medium it grew in and its world's; emptying the flask keeps it. A kept
culture is worked as a world's species: the microscope shows a drop of it growing (`microscopeStageKept`: a tenth of its crowd in that
medium, more of it dividing), the incubator puts it in a flask in that medium, the sequencer reads its species. A kept culture is one
species or a pair (R-440), a pair one entry either way round ("A ROD + A SPHERE"). B-441: a species that grew on its partner's gift
(alone it does not grow in that medium, fed it does) is kept with that partner (`KeptCulture::paired`), staged and put in together; the
pair once (a load merges two kept each way round). The explorer keeps by hand too (R-440, `keepOpen`, `keptAdd`): K on the microscope
keeps the species under the crosshair alone, a copy picked out of the drop (a vial's in its place's medium, a culture's in its own); K on a
flask its one species, or of two either alone or both; each into a folder chosen (UNSORTED, the explorer's, A NEW FOLDER: N, its name
typed, the copy kept in it), never twice in a folder; the vial, the flask and the folder keep theirs. A fossil, a protocell and a dead
culture are not kept. The explorer sorts them: UNSORTED, then the explorer's folders (N makes one, R renames it, M moves a culture, C
copies it, Backspace removes an empty folder or discards a culture). The save's `keptfolder` and `kept` lines (a partner's six fields
after its time).

## The genomes and the tree (`life/sequences.h`, `game/sequencer.cpp`, `game/tree_screen.cpp`, `peoples/made.h`)

Z-06: a species' genome is a pure function of the species and its genesis. Its alphabet is the genesis's (`alphabetOf`: ACGT, six with
Benner's Z and P, eight with hachimoji's S and B: Hoshika et al. 2019), its helix's hand the genesis's (a mirrored life's turns left), its
length its plan's and seed's (`genomeLetters`: a small cell's 0.6-12 million letters, a large cell's 15-900, a colony's 30-2,000).

**The lineage** (`Lineage`, `lineageOf`) is a tree in closed form, from the beginning down, on salts of its own (nothing of generation
17 is drawn again): the genesis's deep branches (`Microbe::clade`) part at 0.93-0.98 of its age; a kin cell's lines part as their cluster
spread (Adams and Spergel 2005), by where their stars are: its sixteen-sector boxes at 0.86-0.90 of its age, the four-sector boxes within
one at 0.78-0.84, the stars within one at 0.70-0.76 (a system of its own genesis: none, its deep branch the system's line); a system's
worlds part at their last exchange of rock (Melosh 1988), 0.5-0.8 of its line's age, a drifter's world one of the star's it left
(`drifterPast`); a world's species form a comb of their deep branch (their order by their seeds, each two neighbours' height 0.06-0.56 of
the exchange: two species' common ancestor the highest between them), home's world's with the ship's own three; a carried spore is its
source's leaf, apart since the impact that carried it. `commonAncestorGyr` reads it; within a deep branch the distances rank as the
lineage does (a world's nearest, a system's, a cell's: the unit's check over the 6,587 species within six sectors, none out of order) and
none across geneses. Home's water life began 7.6 billion years ago: its deep branches parted 7.3 billion years ago, its cell's lines 6.6,
its sixteen-sector box's 6.2, its four-sector box's 5.5, home's worlds' last exchange 3.8 (the kin near home, `lab kin`, at 6.2 and 6.6).

**The marker** (`markerLetter`, `markerOf`; 1,536 letters, 16S rRNA's length): a place's letter is the genesis's ancestor's, changed on
each branch down to the species where the branch's hash at that place falls under its length's chance (0.12 a billion years at the
place's rate; a third of the places never change), so two kin share their letters as far back as their common ancestor (92% alike on one
world, 82% in one system, 66% across stars within a deep branch).

**The sequencer** (the state `SEQUENCER`, E at it on the bow bench; its row magenta, home 47): Enter chooses a species
seen in a vial or growing in a flask (never a fossil's stone or a protocell's chemistry) and reads it in 10 seconds on the lab's clock
(`SEQUENCE_S`, R-449; 90 before), the screen open or not: its letters as they are read in its genesis's dyes under a chromatogram's peaks,
its helix turning its own way. Its end, as the clock reaches it (`sequencerTick` in `updateLab`): the sheet's kin (`SEEN_KIN`), its line
(the genome's letters, its hand, its nearest kin read and their common ancestor or no common ancestor and the geneses met, a made one's
watermark) said and logged (`KIN`). Its `seq` line keeps what is on it, when it began and whether it was told. The sheet reads `SEQUENCED:
ITS KIN ON THE TREE` and lists THE SEQUENCER among the ways to more.

**The tree of kin** (the state `TREE`, E at its screen on the port wall, or Tab from the sequencer on the page of what it read; leaf
green, home 46): Y-01 (R-444), a tree a page and no tree of everything, Left and Right between them (`TreeState::pages`): THE SHIP (its
own three, home's life), each world with a species read (its name, else UNKNOWN and its kind), each of the collection's folders holding a
culture (UNSORTED, then the explorer's). Every species read stays in `tree.leaves` in the tree's order (`treeBefore`: home's genesis
first, then by genesis, deep branch, a kin cell's boxes, the line's star, the world, its comb), the geneses counted there (the LIFE page's
`LIFE BEGAN AT LEAST 3 TIMES`); a page shows its own (`treePage`: `tree.shown`), each two neighbours' common ancestor drawn as the tree
is, each genesis its own clade headed by its alphabet, its hand and when it began, on the page's own years (`treeYears`: its oldest common
ancestor and a quarter, in millions under a billion and a half; a stem older than that runs to the edge). A world's page holds its species
read, those seen and not read counted under the head, and the lab's lines of its species (the strains, `guide.strains`) as dotted twigs at
today's edge under their parents, a strain's strain under it; a line goes on as it arises, Backspace takes one off and A puts it back from
a chooser (the guide's `treeoff` lines), a world's own species staying. A folder's page is what it holds (a pair two leaves, a strain kept
without its species a leaf in the species' place, those not read counted), sorted on the microscope's lists. A species with no name is its
look and size (B-443, `treeLeaves`: its form kept with its lineage, `formOfKey`), two of a world alike told apart as B-442's rows are, and
a line too long for its row leaves out its world ("AN ARMOURED CELL, 65 UM ACROSS"; on a world's own page none is said); the sequencer
says a species in its words on the tree (`treeWords`: its title, its kin, the log's KIN). Up and Down choose a leaf: its nearest kin read
anywhere and their common ancestor below (a drifter's the star it left's), a line's species and its move; Enter sets the remote target to
its world's star. In the room the screen shows a tree of every leaf read (`drawTreeWall`).

**The made** (`peoples/made.h`, the peoples' one reader of the living small): on a world of a futuristic or far-future people one of its own
species (alive or asleep) is theirs (`madeSpeciesOf`, by the world's hash; the people and the year of their count, in their last age, the
generator's). Its genome is too regular to have evolved: the sequencer says so and shows its watermark's letters (`watermarkLetters`: its
text three letters a sign, JCVI-syn1.0's way, Gibson et al. 2010) between the marker's rows. The watermark (`madeWatermarkOf`) is a record
of its people written outside their catalogue at its year by a maker alive where it was made (the grammar's root `watermark`, core/
watermark.vg and watermark_far.vg, every word apart: no people's records move, the pinned hash holds): who made it and where, what it was
made for by how it lives (the air made again, a metal gathered, the waste eaten, a sickness hunted, a street lit), a saying of theirs, a
word
to whoever reads it; the far future's a signature left to itself. Sequenced, it goes to the decoder (`guide.watermarks`): its world listed
by its people, the watermark after their records by its year, read with the language their records taught (it teaches none). Over the
hundred nearest home, 3.1% of its long sentences on a repeated frame (the records' aim at a hundred: 5%), every check at zero.

## The genes (`life/genes.h`, `game/editor.cpp`)

Y-02 to Y-04 (R-443; the design `docs/ideas/IDEAS-genes.md` section 1): a species' genes, read from what it already is, a machine that
changes them, genes moved in from its kin and their switches. J-01 (R-448; `docs/ideas/IDEAS-seeded-worlds.md` section 4): the world
genes. No generation change: nothing of generation 17 moves, and a line with no edit
is its species bit for bit (its values, its culture's segments, its slide's kinds: the unit over forty worlds' 349 species), as is one with
every gene switched to its own.

**A genesis's map and code** (`geneCodeOf`, salts of their own): eighteen places, a gene's number each, the same in all its kin (three
vital: its copying, its membrane, its making; three quiet; a warmth, a salt, a size, a pigment, a food, a motion, three senses, a rest, a
glow, a shape), in an order of the genesis's hash; each letter's weight (home's pinned A 0, T 1, C 2, G 3: a G and C rich stem holds at a
higher heat, Galtier and Lobry 1997; another genesis's a shuffle of its alphabet's), and its tables (a short run of letters read as a
number in the alphabet's base: a pigment's two letters, a food's three, a motion's kind's two, a sense's two, a shape's two), every choice
in each at least once, most runs synonyms. J-01: then the fourteen world genes at 19 to 32 (gas, dye, lift, frost, slime, shell, stop,
bind, rot, poison, carrier, host, dry, shield), one each, shuffled on a salt of their own, and their six tables (a gas, a dye, a shell's
mineral, what it rots, a poison, a host), each on its own salt; the first eighteen places and the old tables as before (the unit's
digest of the cell genes, their tables and 7,702 edits over 349 species, the same as the code before J-01 gave). R-453: the dye's
table has eleven dyes (red, crimson, orange, yellow, violet, blue, black, fluorescent, and GREEN and PINK last), drawn as J-01 drew its
nine, then two of its synonyms made GREEN and PINK on a salt of their own, so every other run reads the dye it did; a wild species' run is
picked from J-01's table as before (`dyeJ01`), the next of its dye's runs where its own became GREEN or PINK.

**A species' genes** (`microbeGenesOf`): the three vital, one to three quiet, the warmth, salt and size, then those it uses: a pigment (one
that has one or lives by light), a food (a chemical-eater), a motion (one that moves), a sense for each thing it turns to (the light, or
away from it: a dark-dweller's swimmers' flight, R-438; the food, the field, the warmth; three at most), a rest (one that ends in spores,
cysts or the gatherers' fruit), a glow (a luminous one), a shape (a sphere, a rod, a curved rod or a spiral), and every world gene: 21 to
30 a species, a gap where it lacks one. Each has a working part, the run its effect lies in (warmth 12 letters, salt and motion 6, vital
and quiet 8, size,
rest, glow and shape 4, food 3, pigment and sense 2), its wild letters chosen so that the code gives its own values: a warmth's weight its
best over its cell's range (a heat-lover's heavy, a cold-lover's light), a salt's in its band's third, a size's by its cells' width (three
a doubling about 1 um, 40 a large cell's, 50 a colony's), a table's run one that reads its own pigment, food, kind, target or plan.

**An edit** (`applyEdits`) moves a value from its parent's by the change in the code: a warmth's range a step a weight (its cell's range
over the gene's heaviest, to half a degree: water's 4 C with four letters, 2.5 with six, 1.5 with eight; the cryostat's 0.5), held within
its cell's range or its own; a salt's band; a size by two to the third of the change, within its family's widths; a pace by two to the
quarter; a pigment, a food, a sense's target, a small cell's plan (its outline drawn again for it, its width kept) and a motion's kind (its
plan's: a rod's run and tumble, glide or twitch; still) by the run's choice; a rest or a glow on past a quarter of its weight (a glow's
share of its crowd halved a weight up, 0.5 to 30%). A gene cut loses its work: the warmth only round its best (4 C under, 2 over), fresh
water, half its size, no pigment, no food, still (its flagella kept, paralysed), no sense, dying at its end, no glow, a sphere (Jones et
al. 2001's MreB). A vital gene changed or cut: the line does not live (`CultureKind::dead`: `GF_UNVIABLE`, its cells lyse in about ten
seconds, its drop their ghosts); a quiet one changes nothing. How it lives and its plan's family never move. A strain's genes are its
parent's with its move's gene stepped its way (`strainGenes`: a hotter top, heavier warmth letters), so the sequencer reads which.

**The world genes** (J-01): each a thing a cell makes or does; its world decides what that does to it (from J-03). A table gene's working
part is two letters of choice and four of weight, a weight gene's four; an amount works past a quarter of its weight (`geneAmount`, 0 to
1). Their wild values are read from what a species is (`wildWorld`, `worldOf`): a gas by how it lives and what it burns (the oxygen-makers
OXYGEN, a sea's algae DIMETHYL SULPHIDE, the methanogens and Titan's acetylene-eaters METHANE, the methane-eaters CO2, the nitrifiers
NITROUS OXIDE, the dead-eaters CO2 or, without oxygen, HYDROGEN SULPHIDE or HYDROGEN), a dye where a pigment leaks (red snow, a spring's
band, a pink lake, a red tide), lift from its gas vesicles, frost for a snow, ice or cloud dweller of the melt (at its best above 0 C; a
colder one keeps its water liquid), slime from its capsule, a shell from its wall's mineral or crystals (magnetite RUST, sulphur globules
SULPHUR), stop at its own crowd, bind from its cells' arrangement, rot a
dead-eater's or a rock-borer's, a hunter's poison too weak to work, carrier where it exchanges genes, no host, dry for dry ground, shield
from its radiation's dose (and a dark or red pigment). A table reading NONE keeps a working weight, so one change of its choice shows.
They move no cell value; their edits set a line's `CultureWorld` from their letters, STOP's crowd by twice the change of its weight's
share (-1.5 to +1 decades; cut, it never stops), LIFT, SLIME and BIND the form's vesicles, coat and arrangement (a sphere's clusters, a
rod's chains). A product a species does not make (a gas, a dye, a slime, a shell, a poison) costs its line 2% and up to 4% more by its
amount (`genePrice`); its own nothing. Their own switch is ALWAYS. Against its parent a line shows them as it shows the others: a gas,
a dye, a scum, slime, a sediment, clumps, an overgrowth, ice and growth under UV alone in a flask; a poison and genes passed with another in
it; vesicles, a coat and bound cells in the drop. ROT, HOST and DRY show only on a world.

**The gene editor** (the state `EDITOR`, E at it on the bow bench past the sequencer; deep blue, home 48): Enter
(Tab once one is on) chooses a culture whose species the sequencer read, a flask's or the collection's, its line as it is (another is
refused: SEQUENCE IT FIRST). Its map is two pages (J-01): CELL GENES 1/2, two columns of nine, and WORLD GENES 2/2, two of seven; the
arrows walk across both pages'
columns, Page Up and Page Down turn (Tab already chooses another culture). Each place is NOT KNOWN, its kind once known (R-450: below) or
the explorer's name (N, its species'), a dash where it has no gene, its mark where edited (= changed, - cut out, + moved in, * a copy; ~ its
switch set after it); the
arrows choose a gene, Enter opens its working part (Left and Right a letter boxed, Up and Down it through the alphabet, Backspace its
parent's letter, a moved gene's or a copy's as it came; the letters' keys are not read: P and T are the game's own), C cuts it out and back
(a gene put in a gap taken out again), S its switch (Y-04), Backspace makes it its parent's again. The line under the map names the gene
chosen and what its kind is for (`GENE 5: WARMTH - THE WARMTHS IT GROWS AT`, `GENE_KIND_PURPOSE`; not known, `CHANGE IT TO SEE WHAT IT
DOES`); a gene known shows its value as its letters give it beside its parent's and its kin's. With its letters open, the map's place shows
its guide (`renderEditorGuide`): how its letters set it (`ITS LETTERS' WEIGHTS ADD UP: HEAVIER, WARMER`, `LETTERS 1-2 PICK WHICH, 3-6 HOW
MUCH`), its genesis's weights, heaviest last, its pick and its weight now beside the parent's, and what each letter of the alphabet at the
cursor would make of it (`LETTER 3 SET TO:`, `A BEST 21 C`, `C OXYGEN`: `geneValueShort` of the line as applied, the current one bright).
A vital gene changed says `A VITAL GENE IS CHANGED: THE STRAIN WOULD DIE`. J-15: a cut STOP says what the line overgrows the land as
(`CUT OUT: OVERGROWS AS CARPETS, GREENS A DESERT`, `overgrowthTypeOf`: coral, sponge, bladders, hair, lichen, carpets, fungus, slime mould
or flesh by its other genes, films where it slimes without binding). W writes the strain. It is refused when nothing changed, when
the line would die (a vital gene changed) or no flask is empty. Otherwise it asks two
things (R-445): which folder of the collection the new strain goes in (the keep chooser with `KeepPick::write`; the parent's folder
chosen first, NEW FOLDER for a new one), then its name (`TEXT_LINE`, NAME THE NEW STRAIN, holding the parent's name and the new strain's
number, "VURYM E2"; an empty
name refused). Esc at either writes nothing. The writing then takes 10 seconds on the lab's clock (`EDIT_S`, R-449; two minutes
before), with the screen open or not. When it ends (`editorTick` in `updateLab`), the strain is recorded (the guide's `strain` line,
SM_EDIT with its edits; numbered as its species' next, E for an edited strain), named, kept in the chosen folder at once, and put into the
first empty flask in its culture's medium. It is said and logged (EDIT). Its edits stay on the editor for the next. A line is read
everywhere a strain is: its flask's
culture and its form (`flaskKind` walks its lines from the wild one, a strain's move or an edit's genes each), its drop, the sequencer
(EDITED: what it changed; a strain's gene that differs), the collection as it grows, its twig on its world's tree (`E1 - GENE 11 CHANGED`).
The save's `editor`, `editoredits`, `editorwrit`, `editorwriting` (the folder and name being written) and `editorfrom` lines keep what
is on it, `editorkin` and `editorkinfrom` its kin.

**Genes from others** (Y-03): K puts a kin beside it from a chooser of the species read (a folder a world, its own world's first) and the
cultures; one of another genesis is marked ANOTHER LIFE and refused (DIFFERENT GENE ALPHABET, MIRRORED CHEMISTRY or ANOTHER ORIGIN OF LIFE,
each "- GENES WON'T FIT"), one not sequenced refused. Beside it the map counts each gene's working letters that differ from the kin's (+
where
the kin has one and it has none), the head their genomes' share alike (`markerAlike`), the gene chosen its kin's letters (those that differ
bright) and value: a hint, never an answer. M moves the kin's gene at the place chosen in (with none beside it, the chooser first): its
letters and its value the kin's (`donorValue`: a warmth's range, a salt's band, its cells' width held in its family's, a pace's share of its
kind's in its own plan's kind, a glow and its crowd, a shape's length; a table's choice its letters'), over its own gene, into a gap or a
gene its parent cut out (a rest there its family's way). The guide keeps it as `5+<letters>@<the donor's key>`; `flaskKind` reads the donor
again each time (`geneDonor`, a few lines deep), and the gene keeps whose it was (its origin, not the kin it was carried by) and how far it
came, down its lines. D copies the gene chosen to the first free place of its kind (a gap or one cut out: a second sense;
`14*<letters>@13`), chosen then to be changed by hand (Ohno 1970). Its price (`geneBurden`, `CultureKind::burden`, the best rate's share): a
gene not its own by how far it came, its world's kin 3%, its system's 6%, another star's 10% (Bentley et al. 1990's burden); a motion past
its species' own pace 4% a doubling (5% where it was still); a glow not its own 4%, lit at a smaller crowd 2% a halving (lit by its switch,
as at its least, 0.5%: Y-04); at most half. Its
doubling shows it, and beside its parent in a flask it loses (Gause, Z-05's competition). What a gene needs (`geneNeeds`): a sense its
cells' motion (a still cell's slide kind answers nothing), a food a chemical-eater, a pigment's light a light-feeder (it colours any), a
shape the four small cells' plans, a motion a form that moves. On its world's tree a moved gene is a dotted amber line from its donor's row
to the line's, the line's words GENE 5 FROM its donor.

**The switches** (Y-04; a promoter, Jacob and Monod 1961): S sets the gene chosen's switch, each press the next: ALWAYS, NEVER, IN THE
LIGHT, IN THE DARK, WHEN CROWDED, WHEN STARVING, WHEN COLD (`GeneSwitch`; the panel's line SWITCH, what it was beside it; the guide's
`16~DARK`, or after its letters `5=ACCG~COLD`). A gene's own is its kind's (`geneOwnSwitch`): a glow's WHEN CROWDED (its quorum), a rest's
WHEN STARVING (its ALWAYS works as its own: it rests only as it ends), the rest ALWAYS; it is kept down its lines, a moved gene keeping its
donor's, a copy its gene's. Off (its condition failing, or NEVER) a gene loses its work as one cut and pays no price; its letters and its
switch stay. The conditions: its flask lit; colder than its best; starving, where the line as written cannot grow (its crowd's food spent,
nothing it eats, no light, its medium killing it) but in the cold, which it waits out; its crowd past its quorum's share (its glow's, else
3%). A switched line's culture (`CultureKind::switches`: each switched gene's condition, its work lost off, its price) is laid out by its
state as every culture is, closed-form: a stretch's light and cold fixed by its medium, a law ending where its crowd crosses its quorum's
share, starving judged of the line as written so that no switch turns itself on (`switchedKind`); its rate there is the incubator's DOUBLING
IN. A glow lit by its switch, not its crowd, glows with its first cells while on (`glowAny`: GLOWING: SWITCHED ON). A size, a shape, a
motion or a sense switched show in its drop alone (who eats whom by its genes all on). Its drop is as its flask's state has it when taken
(`switchedLine`, `MicroscopeState::lineState`; a kept culture's as a young culture of it). So a switch is the clever edit: a glow IN THE
DARK lights a dark flask from its first cells, a moved gene WHEN CROWDED pays its price only past its quorum, a costly motion WHEN STARVING
grows at its species' rate and swims at its crowd, a vital gene IN THE LIGHT dies in the dark (a kill switch), a rest IN THE LIGHT rests lit
and dies in the dark.

**Knowing a gene** (R-450; the guide's `gene` lines, `<species key>/G<number>`): a gene is known for its species, and its lines with it, the
moment it is changed on the editor (its letters, a cut, a gene moved in, for its donor's species too, a copy, a switch; `editorEdited`),
and a line put on the editor has its own changes from its species known; a strain read on the sequencer has its move's gene known
(`strainReadLearn`). Another species of the same genesis knows nothing of it. An older guide's lines by genesis (Y-02's, with NOTHING SEEN
and the candidates `GENE 2 OR 8`) are dropped as it loads, and so are a gene's names by genesis. An edited line is still compared with its
parent (`Flask::parentKind`, made with the flask): in its flask alone where it grows and its parent would not or the other way (its warmth,
its salt, its food, its pigment's light), its end (resting or dying), its glow (`flaskLearn`); in its drop its cells' shape, size, colour
(in a bright field), motion (where it moves) and its answers to the light, the food and the field (`scopeLearn`, `scopeAnswerLearn`; its
answers its line's, never its species' sheet's); a vital gene's death at once. What it shows is said with the changed gene known to do it
(`FLASK 1: GROWS AT 29 C, PARENT DIES: GENE 8: WARMTH GENE`; Y-04: a switched line as its flask's state has it, its parent there too,
`flaskLineIn`, a gene switched alone and on there as its parent's). A known gene that shows nothing for want of what it needs is said once
a flask (`geneNeedSay`: GENE 8, A SENSE: NOTHING IN ITS DROP, then A SENSE ANSWERS NOTHING IN A CELL THAT CANNOT MOVE). Each finding is
logged at once (GENE, the lab's notebook) and said in its turn when the status line is free (`geneSay`): what was seen (`FLASK 2: IT GROWS
AT 29 C - ITS PARENT DIES`), then what it says of the genes.

## The seeded worlds

A culture sent down to a world changes it: the layer, the made air, waters, ground and plants, the animals' fates, the overgrowth, the
made kinds and the record are in `14-seeded-worlds.md`.

## The record (`game/catalogue.cpp`, the guide)

Z-07: what was found of the living small, kept and shown. **The guide** (`guide.txt`) keeps what is known: a species seen is a `microbe`
line (its sheet: the parts seen, the forms of its life seen, the stages its cultures showed, its quickest growth and the medium, and the
years its spores slept before a flask woke them), the explorer's names (`name`: a species `<body>/M<index>`, a strain `.../S<n>`, a gene
`G<genesis>/<number>`), the strains and the edited lines (`strain`), what the genes were seen to do (`gene`), the made ones read
(`watermark`), the samples taken (`stat samples`) and their worlds (`sampled`), and (J-02) the worlds seeded (`seeded`) with the guide's
clock (`clock`). The save keeps what runs aboard (the kit, the cold store, the flasks, the sequencer, the gene editor).

**The catalogue** (the state `CATALOGUE`, the guide's THE CATALOGUE; anywhere, as the log): every species with a sheet, the explorer's and a
friend's lent, by world (a world's head, its name or UNKNOWN and its kind; its species by index, each its name or UNNAMED and its form in
the sheet's words: its plan, SPORES, CYSTS or FOSSILS, MADE where it was read so) or, Tab, by kin (the sequenced in the tree's order, each
genesis's head in its colour and words as the tree's; then NOT YET SEQUENCED). Enter shows a species' sheet (the microscope's lines,
`sheetLinesOf`) beside its form drawn live: one specimen in a framebuffer of its own (every bank its way's), the view following it, still
but for its jiggle where its motion was never seen, a scale bar under it. V the next way it was seen in (the light ways on any slide, the
dark field its flagella, the glow its DNA, the electron its wall; a fossil's the section), Up and Down the next species; N names the one
chosen (`TEXT_SPECIES`), on the list or its sheet. Its worlds are drawn as it opens (`LabSystems`), the names read as it draws.
J-12: by world, the animals seen on a world follow its species (their words or names and the marks the ground showed: GONE, DYING,
CHANGED), each with a sheet in words, and N names one (`TEXT_ANIMAL`). On a seeded world the head names its made kind, the natives a line
drove out read DIED OUT and the lines found wild there follow them, SEEDED (`14-seeded-worlds.md`, "The record"). A species gone from its
world is drawn dim.

**The LIFE page** (L on the statistics): the samples taken and their worlds; the species seen, their worlds and those named, the forms
and the stages seen in the sheet's words; the woken (THE LONGEST AFTER ABOUT 230 MILLION YEARS, the flask's words); the strains; the
sequenced and the made; the tree's count of geneses in its words; kin across stars (the species read whose kin were read at another star,
the farthest two stars apart). Every count is the explorer's own and agrees with the other screens (the catalogue's rows, the tree's
leaves); a friend's lent sit on a cyan line of their own. J-12: once a world is seeded, WORLDS SEEDED, MADE and DIED OUT follow
(`14-seeded-worlds.md`). **The data sheet** of a world: `MICROBES  4 SPECIES SEEN, 2 NAMED, 1 SEQUENCED`
(the catalogue's rows of it), or `SAMPLED, NONE SEEN YET`. **The log**: SAMPLE (each take: its microhabitat, its world, its reading and
activity; the scoop's), FORM, WOKEN, STRAIN, KIN, EDIT, GENE. **The gallery**: a micrograph is P on the microscope's slide, its
caption the slide's.

**Lending** (the guide's IMPORT AN INBOX FILE): the friend's sheets of the species they saw (their `microbe` lines; never those lent to
them, nor one the explorer saw, whose own sheet wins; an earlier loan's added to) become `lentmicrobe` lines, their names coming with the
inbox's. They show in the catalogue in cyan (their form drawn as they saw it, LENT BY A FRIEND'S INBOX) and in no count. Samples and
flasks never travel.

**Auto-explore's samples** (its water outing): it walks to the last dry ground before the water, eases the crosshair a metre down toward it
and presses O once, the take's line as the explorer's; on a world with springs it goes first to the nearest spring's pool within 450 m (a
terraced spring's vent, a chemotroph's hot pool, where its landform put the pool), samples at the rim and looks over it. With the kit
full it takes none; the lab's work stays the explorer's (it opens no screen).

## Determinism, cost and the cache

The biota is a pure function of the system and its bodies' traits (`BodyGen::make`), on salts of its own after every draw of
generation 16: no star, system, body, map or ground moves (regress and `fingerprint bodies` the same). A slide is a pure function of
its view, its specimens and the clock, its rows pure: the same skipped to and at one thread. A slide at 4x takes about 10-20 ms (a
plate's 186 panels in 3.6 s with their pictures written). The microscope's densest slide at 4x (`lab time`): about 10 ms in the light
ways (the bench's `microscope 4x`, budgeted 15.6), 25 to 45 ms under the electron over large cells. `lifeOf` draws a system's
together and keeps the last sixteen, keyed by the star and every body's seed, kind, parent and glassing; its pointer keeps them
alive while held. A system made and its biota drawn take about 30 microseconds (the survey's 1,500 in 0.05 s). A culture is laid out once
when its events change (a few microseconds) and read by a binary search; the unit holds its state the same skipped, stepped and laid
out again from its saved events. A lineage takes a few microseconds (its world's comb sorted), a marker of 1,536 letters about a tenth of a
millisecond; `lab kin`'s ranking over 6,000 species and their 200,000 pairs in about three seconds.

## The harness

`life list [n]`, `life survey [systems]`, `life kin [ly]`, `life plates [n]`, `life strips [plan]`, `life distinct [species]`,
`life forms [plan]`, `life search [scene|all]`, `life kit`, `life made [n]`, `life auto`, `lab microscope`, `lab time`, `lab cultures`,
`lab tree`, `lab kin [r]`, `lab catalogue`, the `seed` and `overgrowth` modes (`14-seeded-worlds.md`)
(`08-testing-and-tools.md`), `fingerprint life`
(`harness/proof.md`), the unit's `life`, `search`, `lab` and `seeding` groups (`harness/unit-checks.md`), the battery's `fp_life`,
`life`, `life_look`, `lab` and `seed` items.
