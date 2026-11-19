# The living small

Microorganisms on every world (R-425, 1.5.0's Z series; the design `docs/ideas/IDEAS-small-and-spacious.md` section 1). `src/life/`
reads `src/core` and `src/galaxy` alone. Z-01 made the biota (generation 17), Z-02 its forms and the slide they are seen on, Z-03 the
search (the samples, the kit, the ultraviolet, red snow and the springs' bands), Z-04 the microscope (a vial's slide, what is done to it,
what is seen of each species), Z-05 the incubator (cultures in twelve flasks, their stages, spores woken, two in a flask, strains) and the
jars, Z-06 the genomes, the sequencer, the tree of kin and the organisms the late peoples made, Z-07 the record (the catalogue, the LIFE
page, lending, auto-explore's samples). Then, on the user's word: R-432 the microscope's lists by world and the cold store without limit,
R-433 the culture collection, R-434 the activity vision.

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
- The incubator (`life/cultures.h`, `life/jars.h`, `game/incubator.cpp`)
- The genomes and the tree (`life/sequences.h`, `game/sequencer.cpp`, `game/tree_screen.cpp`, `peoples/made.h`)
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
mean temperature the year round (`placeMeanC`: the site's formula without the season and the day; a spring's band, a pool's, a
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

**The events** (`SlideEvent`, on the slide's clock): `L` a spot of light at the crosshair (a ring on the HUD, the field brighter in
it), `M` the magnet beside the stage and then turned half round, `H` the stage warmed 30 C over the vial's (eased in and out, 12 s),
`F` a drop of food at the crosshair. `slideStateAt` folds them; `mountSpecimens` gives the specimens in a view at a time, closed-form:
a copy that turns to light or food (`TX_LIGHT`, `TX_FOOD`), alive, moving, within its reach (four radii of the spot, or what it swims
in 25 s, to twelve) and of the share that makes a crowd a cell deep (`answering`) runs to a point of it at its speed, its wander
narrowed as it goes (`Specimen::wander`, its heading turned by its drift `vx`, `vy`); a magnetotactic one swims along the field from
the magnet's coming, a U-turn of 1.6 s at each turn (its side alternating, so its sideways steps cancel); a kind stressed when the
warmth passes its top (+2 C) does its act from where it is (2-12 s apart) and ends still in its form. A spore, a cyst, a cell that
does not grow at the warmth, a microcolony are still (`Specimen::still`: the jiggle alone, the beat stopped). The depth's blur
(`Specimen::blurUm`: past half the depth of field, `depthOfField(mag)`: 25 um at x40, about one at x1,000) spreads a cell's edges and
fades it; a copy whose own extent (its wander placed) misses the view is left out; past 3,000 of a kind in a view an even share of its
copies is drawn. Under the electron the slide's clock stops (a fixed sample).

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
wall, the forms it was seen as) and the ways that would show more. `P` takes a micrograph to the gallery (the photograph's key, its
caption the way, the magnification, the species and the vial).

## The incubator (`life/cultures.h`, `life/jars.h`, `game/incubator.cpp`)

Z-05: the laboratory's second device, a screen over the room (`E` at its cabinet on the stern bench; the state `INCUBATOR`, its row
amber, home 20). Twelve flasks behind its glass door, three shelves of four, each a medium and a culture of one or two species; the
jars' shelf over the bow bench. All of it runs on the lab's clock (`LabState::clock`, the drones' kind: real seconds while the game
simulates, saved; none in the menus).

**A flask** (`Flask`, `lab_state.h`): its species (`FlaskSpecies`: a vial's world and the species' index there, its strain now and the one
it went in as), its medium (`Medium`: its cell, water's or the acid cell, the ammonia cell, the cryostat, each its chemistry and its range;
its warmth; its light, dark, white, red or blue; its energy, a substrate or the broth; its salt), its events on the lab's clock
(`CultureEvent`: started, the medium changed, a fresh broth, a second added, a strain) and AS ITS WORLD (`mediumOfPlace`: the vial's
chemistry's cell, the warmth read at the take, the sun's light where it reached the place, the place's energy, a spring's sulphide, a
plume's hydrogen, a rust world's iron, else the broth; its salt), the first guess a species goes in at. Enter puts in a species seen in a
vial (only what the microscope showed of it: never a fossil or a protocell) or a flask's culture; on a culture a second.

**A culture** is closed-form from its events (`runCulture` lays its laws out as segments, `cultureAt` reads one): a lag (Baranyi and
Roberts 1994), the logistic rise at the medium's rate (the species' cardinal temperatures, Rosso et al. 1993; its energy; a light-feeder's
light by its pigment; a salt one step out its range's edge), its crowd by its energy's yield and its family (a small cell's 10^9, a large
cell's 10^6, a colony's 10^5), held a few minutes at it (`AT ITS CROWD`, or `STARVING` with nothing it eats) and then its decline: a spore
former sporulates, a large cell encysts, the gathered amoebae gather and fruit, the rest die (lysed: the drop's ghosts). Too hot, the
wrong salt or solvent kill at once (a spore former's spores among them); too cold it waits. A species' pace comes from its seed (its
crowd at its best in 8 to 20 minutes, its lag, its spores' waking 20-60 s, its hold). Spores wake once the medium suits them and feeds
them, most of them, the rest asleep (`WOKEN AFTER ABOUT 12,000 YEARS`: its world's end, its drifter's leaving or the impact that carried
it); a luminous crowd glows (from 3% of its crowd: Vibrio's quorum). The stages a curve passes are each species' own (`CultureStage`).

**Two in a flask** (`pairOf`): mirrored hands ignore each other (each its own crowd); a grazer or a predator and a smaller cell cycle (the
prey alone till the predator reaches its measure, then a damped Lotka-Volterra turn about its balance, the prey a decade down; the turn
2 pi over the root of the prey's rate and the predator's death); a light-feeder and a heterotroph, or a fermenter and a hydrogen-eater,
feed each other (the fed one held to a share of its feeder); the rest compete (Gause 1934: the one at its crowd, else the quicker, wins;
the loser falls once the winner holds half its crowd). The screen says the pair once its curves show it.

**Strains:** a culture growing at its range's edge (its warmth past half its rate, a salt one step out, a light its pigment takes poorly)
counts its generations; at twelve a strain arises (`STRAIN_GENERATIONS`, Lenski and Travisano 1994): its highest warmth up, its lowest
down or its cells a quarter bigger (Atkinson 1994), its salt taken in, or its pigment the light's. The clock passing its time appends the
event (at that time) and the guide's `strain` line (`<body>/M<index>/S<n>`, its parent and its move); N names it (`TEXT_STRAIN`).

**The screen:** the flasks (each its species, in its stage's colour), the chosen one's curve on a log scale since it was started (a
pair's two, spores dotted), its species' stages and pace, its pair, what it came to (woken, a strain, the glow), its medium (the field
chosen lit: Left and Right, `+`/`-`, Shift ten degrees; W as its world again; F a hundredth into fresh broth, a serial transfer;
Backspace empties it); Tab the jars. The microscope's list holds the flasks after the vials: Enter puts a drop of the culture as it is on
the stage (its cells, a quarter of them dividing while it grows, its spores, its lysed, the gatherers' slugs and stalks). What the curve
shows of a species goes to its sheet (`MicrobeSheet::stages`, its quickest doubling seen and the medium of it: the sheet's `STAGES SEEN`
and `GROWS` lines). In the room the flasks' liquids take their culture's tone (its pigment's, a broth's cloud), lit through by their light,
aglow with a crowd's own; their tones and the jars' are homes past the banks (`ShipBanks`' 32 to 47), dealt a free bank while in view.

**The jars** (`jarRecipe`, `jarAt`): a vial of wet silt, a mat or soil in a column, a world's one (eight places): the silt settles in its
first minutes and its living species come up in bands by what each lives on (the oxygen-making light-feeders at the top, the others of the
light under them, the iron's rust, the sulphide's white, the deep's grey, the black of the anaerobes at the foot), each by its crowd,
all in by twenty minutes (Winogradsky's column).

**The collection** (R-433, `game/lab_lists.cpp`): every species and strain a flask grows, kept frozen as labs keep their stocks. A
species is kept once its curve passes its lag (`cultureStagesBy`'s GROWING), a strain as it arises (`keepCulture` from `updateLab`;
the flask's `FT_KEPT` bits say so), each once, with the medium it grew in and its world's; emptying the flask keeps it. A kept
culture is worked as a world's species: the microscope shows a drop of it growing (`microscopeStageKept`: a tenth of its crowd in that
medium, more of it dividing), the incubator puts it in a flask in that medium, the sequencer reads its species. The explorer sorts it:
UNSORTED, then the explorer's folders (N makes one, R renames it, M moves a culture, Backspace removes an empty folder or discards a
culture). The save's `keptfolder` and `kept` lines.

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

**The sequencer** (the state `SEQUENCER`, E at it on the bow bench under the jars' shelf; its row magenta, home 47): Enter chooses a species
seen in a vial or growing in a flask (never a fossil's stone or a protocell's chemistry) and reads it in 90 seconds on the lab's clock
(`SEQUENCE_S`), the screen open or not: its letters as they are read in its genesis's dyes under a chromatogram's peaks, its helix turning
its own way. Its end, as the clock reaches it (`sequencerTick` in `updateLab`): the sheet's kin (`SEEN_KIN`), its line (the genome's
letters, its hand, its nearest kin read and their common ancestor or no common ancestor and the geneses met, a made one's watermark) said
and logged (`KIN`). Its `seq` line keeps what is on it, when it began and whether it was told. The sheet reads `SEQUENCED: ITS KIN ON THE
TREE` and lists THE SEQUENCER among the ways to more.

**The tree of kin** (the state `TREE`, E at its screen on the port wall, or Tab from the sequencer; leaf green, home 46): every species read
and the ship's own three at its root, in the tree's order (`treeBefore`: home's genesis first, then by genesis, deep branch, a kin cell's
boxes, the line's star, the world, its comb), each two neighbours' common ancestor drawn as the tree is (billions of years back, each
genesis its own clade headed by its alphabet, its hand and when it began); the geneses counted (`LIFE BEGAN AT LEAST 3 TIMES`). A drifter's
species sits beside the star it left's once both are read. Up and Down choose a leaf (its nearest kin and their common ancestor below);
Enter sets the remote target to its world's star. In the room the screen shows a tree of its leaves (`drawTreeWall`).

**The made** (`peoples/made.h`, the peoples' one reader of the living small): on a world of a futuristic or far-future people one of its own
species (alive or asleep) is theirs (`madeSpeciesOf`, by the world's hash; the people and the year of their count, in their last age, the
generator's). Its genome is too regular to have evolved: the sequencer says so and shows its watermark's letters (`watermarkLetters`: its
text three letters a sign, JCVI-syn1.0's way, Gibson et al. 2010) between the marker's rows. The watermark (`madeWatermarkOf`) is a record
of its people written outside their catalogue at its year by a maker alive where it was made (the grammar's root `watermark`, core/
watermark.vg and watermark_far.vg, every word apart: no people's records move, the pinned hash holds): who made it and where, what it was
made for by how it lives (the air made again, a metal gathered, the waste eaten, a sickness hunted, a street lit), a saying of theirs, a word
to whoever reads it; the far future's a signature left to itself. Sequenced, it goes to the decoder (`guide.watermarks`): its world listed
by its people, the watermark after their records by its year, read with the language their records taught (it teaches none). Over the
hundred nearest home, 3.1% of its long sentences on a repeated frame (the records' aim at a hundred: 5%), every check at zero.

## The record (`game/catalogue.cpp`, the guide)

Z-07: what was found of the living small, kept and shown. **The guide** (`guide.txt`) keeps what is known: a species seen is a `microbe`
line (its sheet: the parts seen, the forms of its life seen, the stages its cultures showed, its quickest growth and the medium, and the
years its spores slept before a flask woke them), the explorer's names (`name`: a species `<body>/M<index>`, a strain `.../S<n>`), the
strains, the made ones read (`watermark`), the samples taken (`stat samples`) and their worlds (`sampled`). The save keeps what runs aboard
(the kit, the cold store, the flasks, the jars, the sequencer).

**The catalogue** (the state `CATALOGUE`, the guide's THE CATALOGUE; anywhere, as the log): every species with a sheet, the explorer's and a
friend's lent, by world (a world's head, its name or UNKNOWN and its kind; its species by index, each its name or UNNAMED and its form in
the sheet's words: its plan, SPORES, CYSTS or FOSSILS, MADE where it was read so) or, Tab, by kin (the sequenced in the tree's order, each
genesis's head in its colour and words as the tree's; then NOT YET SEQUENCED). Enter shows a species' sheet (the microscope's lines,
`sheetLinesOf`) beside its form drawn live: one specimen in a framebuffer of its own (every bank its way's), the view following it, still
but for its jiggle where its motion was never seen, a scale bar under it. V the next way it was seen in (the light ways on any slide, the
dark field its flagella, the glow its DNA, the electron its wall; a fossil's the section), Up and Down the next species; N names the one
chosen (`TEXT_SPECIES`), on the list or its sheet. Its worlds are drawn as it opens (`LabSystems`), the names read as it draws.

**The LIFE page** (L on the statistics): the samples taken and their worlds; the species seen, their worlds and those named, the forms
and the stages seen in the sheet's words; the woken (THE LONGEST AFTER ABOUT 230 MILLION YEARS, the flask's words); the strains; the
sequenced and the made; the tree's count of geneses in its words; kin across stars (the species read whose kin were read at another star,
the farthest two stars apart). Every count is the explorer's own and agrees with the other screens (the catalogue's rows, the tree's
leaves); a friend's lent sit on a cyan line of their own. **The data sheet** of a world: `MICROBES  4 SPECIES SEEN, 2 NAMED, 1 SEQUENCED`
(the catalogue's rows of it), or `SAMPLED, NONE SEEN YET`. **The log**: SAMPLE (each take: its microhabitat, its world, its reading and
activity; the scoop's), FORM, WOKEN, STRAIN, KIN. **The gallery**: a micrograph is P on the microscope's slide, its caption the slide's.

**Lending** (the guide's IMPORT AN INBOX FILE): the friend's sheets of the species they saw (their `microbe` lines; never those lent to
them, nor one the explorer saw, whose own sheet wins; an earlier loan's added to) become `lentmicrobe` lines, their names coming with the
inbox's. They show in the catalogue in cyan (their form drawn as they saw it, LENT BY A FRIEND'S INBOX) and in no count. Samples, flasks
and jars never travel.

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
`lab jars`, `lab tree`, `lab kin [r]`, `lab catalogue`
(`08-testing-and-tools.md`), `fingerprint life`
(`harness/proof.md`), the unit's `life`, `search` and `lab` groups (`harness/unit-checks.md`), the battery's `fp_life`, `life`,
`life_look` and `lab` items.
