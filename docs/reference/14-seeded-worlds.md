# Seeded worlds

A culture grown aboard and sent down by a drone changes the world it lives on (R-448, 1.5.0's J series; the design
`docs/ideas/IDEAS-seeded-worlds.md`). J-01 gave every species fourteen world genes (`13-life.md`, "The genes"). J-02 made the layer a
seeded world is drawn through, J-03 sends a culture down and shows the first change, J-04 changes the air and the climate, J-05 the
waters, ground and plants. J-06 and J-07 drew the animals and their ways (`06-surface.md`), J-08 their fates. J-09 grows the overgrowth,
J-10 lets it take the ruins and gives it sounds, J-11 names the made kinds, J-12 keeps the record. R-454's J-13 to J-15 open the cold
worlds, the clouds and the deserts to it. A world nobody seeded has no layer and
is drawn as generated, bit for bit. The rules: `../decisions/rules-life-world.md` (the genes and the layer), `rules-life-seeding.md`
(sending and the record), `rules-life-made.md`, `rules-life-overgrowth.md` and `rules-life-kinds.md`.

<!-- contents -->
Contents:
- Sending a culture down
- Whether it takes
- The cold worlds and the deep (J-13)
- The clouds (J-14)
- The drift
- The layer
- The made air (J-04)
- The made climate (J-04)
- The made waters, ground and plants (J-05)
- The animals' fates (J-08)
- The overgrowth (J-09)
- Ruins overgrown, and the sound (J-10)
- Made kinds (J-11)
- Greening a desert (J-15)
- The record
- What the explorer is told
- Where it is read
- The harness
<!-- /contents -->

## Sending a culture down

Parked in orbit of a landable world, S on the drone bay opens the seed page (the state `SEEDING`): the
world's map with the ground under the ship and the places of its lines, the culture (C: the next culture alive in a flask, B-473, its
stage said; it opens on the one last chosen, else the first that shows FULL) and what it needs (R-451: its liquid, food, salt, warmths and
whether it lives dry, `seedNeedsWords`), the place (the arrows, two degrees a press, ten with Shift) and what lies there, read at the world
map's scale (`seedPlaceOf`: the sea, a lake, the sea shore, snow, ice, a salt crust, soil, sand, bare rock and the rest, as a vial there
would be named), and what it gives a culture (R-451: the verdict's first part alone, `seedPlaceFacts`, in `seedPlaceWords`: no forecast).
J-13: Page Down takes the place down a drill hole and Page Up back toward the ground (`seedDepthsOf`; below); J-14: Page Up goes on up
into a cloud deck (`seedHeightsOf`).
Enter sends drone 1 alone. It falls for about sixteen seconds, lets the culture go as it arrives (the guide's `seeded` line, SEEDED in the
log), watches two minutes and climbs; its report is the verdict ("UNKNOWN: CULTURE GROWS", "CULTURE DIED, TOO COLD"), then its reason in
full (R-451, `seedWhyWords`: "TOO COLD: IT GROWS FROM 7 C, THE PLACE IS -40 C", "NO FOOD: IT EATS DEAD MATTER, THE PLACE GIVES HYDROGEN"),
in the log and on the bay's list too (the save's `bayseedwhy`). The flask keeps its
culture, and the bay is ready again at once (R-449). A save keeps a trip in flight (`bayseed`). Refused: at home (the pinned home star's
first felisian planet, where every game starts), at a giant, out of orbit, with the drones away, and where three lines live.

## Whether it takes

`seedVerdictOf` is a pure function of the world, the line and the place, checked in order, the first failure
said: a furnace; the medium, the incubator's own (`mediumOfPlace` at the place's mean warmth, judged by `growthIn`: the wrong liquid,
too hot, too cold, the wrong salt, no food, no light), its food a native it eats or one that feeds it; oxygen (an oxygen air kills a line
that bears none, outside a sediment, a mat or a spring; a line that needs it dies without it); pH (an acid or ammonia world's, the rust's,
a spring's, a soda lake's, the sea's 8, fresh water's 7.2, the ground's 6.8, half a unit of grace); the year's dose (1 Gy under an air
with a field, ten times without a field, 300 times on an airless world, three times under a hot star, less under water, ice, rock and
soil), which SHIELD raises 41 times at its full; land beyond the water needs DRY; and the natives at the place (the sample there, each
taken as a flask takes it: `pairOf`). A competitor that grows there a tenth faster outgrows the line (its genes' price is in its rate);
one slower is outgrown, and between them they live side by side; a line eats its prey, and its poison kills what it meets. `seed worlds`
sends six lines of one sea light-feeder to forty worlds: 21 of 240 seedings take, most of the rest dying of the cold.

## The cold worlds and the deep (J-13)

R-454's first milestone. A place is read at its own warmth: a flask's cell holds water from -20 C, but a world's ground can be -200 C,
and the verdict once read such a place as -20 C. Colder than -20 C only a brine stays liquid, so such a place's water is brine. Water life
grows down to -20 C (`WATER_COLDEST_C`); a line that lives in brine (its salt's band reaching BRINE) and makes no ice of its own (its
FROST none, cut or switched off: `brineAntifreeze`) down to -45 C (`BRINE_COLDEST_C`), as perchlorate brines stay liquid far colder
(Toner et al. 2014). `applyCore` reads it once every edit is in, so the salt, the frost and the warmth may be edited in any order; a band
edited lower is lifted whole as far as its own (or a moved-in donor's) went. A brine flask goes down to -45 C in the incubator. The
editor's warmth, salt and frost guides (letters open) say how cold the line can go and what it lacks for -45 C (`coldGuideWords`).

Down a drill hole: the seed page's Page Down takes the drone down the rock's eight depths, 1, 2, 3, 5, 8, 12, 20 and 30 km
(`SEED_DEPTH_KM`). On a small cold world, whose rock warms slowly down, they stretch to the depth where its rock is 10 C warm under its
mean, 120 km at most (B-478: `deepestKm`; a 767 km rocky world's eight reach 54 km). The rock's pores hold water there, sea-salt (brine
colder than -20 C), at the ground's warmth (never colder than its
reading) plus the world's inner heat: 30 C a kilometre on an Earth, by the radius as a world's heat flow goes, half again with plates or
vents or as a giant's moon (`deepGradientC`). It is dark, its food the rock's hydrogen (the deep biosphere's: Stevens and McKinley 1995),
with no oxygen, pH 8 and the rock's own radioactivity alone, whatever the world's liquid. A world with a sea under its ice (europan, brine
dwarf, grooved ice, tiger-striped) has that sea alone, at the ice's foot, 3 to 25 km down (2 to 8 on a tiger-striped moon; `iceShellKm`),
1 to 4 C, salt (a brine dwarf's brine). A furnace stays closed at any depth. Its natives are the rock's or the plumes' that live in water.

A deep line (`MadeSpread::deep`) lives in no water and on no land of the surface, grows over nothing and touches no animal; its gases
seep up to the air at half its share (`DEEP_GAS_SHARE`; its share the points whose depths hold its band of warmth). Under the ice its
dye colours the bare ground its sea comes up through, the lineae and cracks (RED CRACKS). In the rock its dye stains the bare rock in
patches of a few hundred metres where its water seeps up (B-479: RED SEEPS, `MGT_COLOURED_SEEPS`, in an ochre seep's colour under the
dye's); a glowing deep line's cracks or seeps glow at night (GLOWING CRACKS, GLOWING SEEPS: `landsGlow`), in their own stain's colour
where it has no dye. It is found in a plume's vial alone (a sea under
the ice); a native it beat dies out if it lives in the deep's niche alone. A `seeded` line ends with its depth (km) when it went down
(`bayseeddepth` in a save while the drone is out); the data sheet's SEEDED line says `5 KM DOWN` or `UNDER ICE`.

`seed cold` sends lines tuned as a player would tune them (`seedTunedEdits`: the warmth's best nearest the place's, brine and the frost
cut where only a brine stays liquid, DRY where the ground is dry, SHIELD where the dose asks): of forty worlds the wild sea light-feeder
takes on 4; tuned, a light-feeder, a hydrogen eater or a fermenter takes on the ground of 24 and in the deep of 6 more. Venus is too hot
at every depth (its clouds are J-14's). The two smallest cold worlds (a rocky world of 767 km, a carbon one of 903), still about -75 C
at 30 km in J-13, open 41 and 50 km down (B-478). The furnaces (7) stay closed.

## The clouds (J-14)

R-454's second milestone. A world with a cloud deck life lives in (its biota's cloud niche: the venusian and the acid worlds; never a
furnace: `seedCloudDeck`) has its heights in the clouds: where the air is 70, 60, 50, 40, 30, 20, 10 and 0 C (`CLOUD_LEVEL_C`), read
from the world's mean at the equator as generated, 8 C colder a kilometre up (the capsule scoop's lapse), from the clouds' base 3 km up;
a cool acid world, whose air is under 0 C there, has its base alone (`seedHeightsOf`). A venusian world's eight lie some 38 to 53 km up.
The seed page's Page Up climbs into them, a height a press ("THE CLOUDS 47 KM UP"), refused past the highest (AS HIGH AS THE CLOUDS GO);
its footer says PGUP CLOUDS on such a world. A height's place: the ground's warmth at its level and the made warmth, 8 C colder a
kilometre up; acid droplets whatever the world's life (Venus's are sulphuric acid: Seager et al. 2021), lit by the star, fresh, dead
matter, iron and sulphide (the scoop's vial of the clouds holds the same; B-480: an acid's water carries both), the air's dose, never
dry; its natives the clouds' (the scoop's own cubic metre,
a kilometre's layer). Acid life brought from such a world lives there; water life never does (WRONG LIQUID: no gene changes a line's
liquid). A cloud species that burns iron or sulphide finds its food there, as in any acid medium (B-480: `acidFeeds`).

A cloud line (`MadeSpread::deep` `MADE_CLOUDS`) lives in no water and on no land of the surface, grows over nothing and touches no
animal. Its share is the deck's round the world, wherever its band of warmth lies between the deck's lowest and highest heights over a
point, all of it in the air. Its dye colours the clouds (RED CLOUDS) and its glow lights them at night (GLOWING CLOUDS) at its whole
answer, with no lift; its spores make spore clouds; its gases change the air as any line's. A vial of the clouds (the scoop's) within
its band finds it; a native it beat dies out if it lived in the clouds alone. A `seeded` line's depth is negative for a height (the
save's `bayseeddepth` too); the data sheet's SEEDED line says `47 KM UP`. From orbit a venusian world's deck takes the dye in dark
streaks along the latitudes, drifting with the deck, from the equator to the middle latitudes (Venus's own: Limaye et al. 2018), and the
glow over its night side, brightest where the deck is thickest (`02-rendering.md`); an acid world's clouds take J-04's made clouds. On a
venusian world's ground the deck's underside is the sky: by day the dye moves the sky bank's whole ramp toward its colour, and at night
the glow lights the deck in its form, 12 km over the ground, in the made clouds' bank (B-481).

`seed clouds` prints the heights of the first venusian worlds and the first acid world and what each gives, a venusian cloud
light-feeder wild and tuned at each and the sea light-feeder of `seed worlds` refused at each (WRONG LIQUID), and lets the light-feeder,
tuned, dyed red and given a glow moved in from a glowing species (`seedGlowDonor`), go in the clouds: a venusian world's globe before and
at its 24th step by day and by night, its data sheet, an acid world's globe, and the seed page in the clouds (`clouds/`).

## The drift

A line that takes can change in the wild: one or two of its genes (never a vital one) stepped a letter or broken, by a
hash of the world's seed, the line's key and the place, in about a third of seedings, twice as often under a strong ultraviolet. The
spread and its looks are the changed line's. The sequencer reading the line from a vial of that world says CHANGED IN THE WILD: GENE n.

## The layer

`madeWorldOf` gives a world's layer from its seedings at the guide's clock: the first three that took, each a spread round
its place, 2 km at step 1, 150 km at step 6, the whole world at step 24, a step each 75 seconds at the line's pace (R-449: J-02 had five
minutes, so a world takes half an hour where it took two hours; its pace by its doubling there against half an hour's, half to one and
a half, times one plus its LIFT). Within its reach a spread keeps to where its line lives: the waters
of its salt (as `growthIn` passes them), the land where it lives dried out (or in snow and ice), its band of mean warmth (`placeMeanC`).
A dye colours the waters a line lives in, a rust of the dye's colour mixed into the world's water (the latest dye's, from the first step;
R-453 added GREEN, a green alga's, and PINK, a salt lake's);
a glowing line lights them at night. The data sheet names the traits ("RED SEAS", "GLOWING SEAS", or "WATERS" on a world without seas).
A layer that draws nothing (lines that only spread, or died) has key 0, so no map is made again for it. The layer's key hashes every field.

## The made air (J-04)

What the lines that took give a world's air (`madeAirOf`, with the layer): each gas by its gene's amount, times
the share of the world its line lives on over 0.15 (`SeedVerdict::share`, read at 96 points over the sphere: its waters, its land and the
plants a host of plants or a wood rotter lives in), or of its room where that is less (B-475: `SeedVerdict::room`, its kind of place's
share at any warmth: its waters of its salts, its land, its plants; a terminator world's ring of seas a hundredth of it), rising from
the line's step 6 to its step 24, so the air changes after the looks. A line that took lives at least at the point it went down. Its
share is read again at the warmth its own gas makes with the lines before it, and kept where that is more (B-476): a line that opens its
world to itself spreads and gives off more, one whose warmth drives it back keeps the air it made.
The world answers by what it is, a gas leaving an airless one. CO2 warms (20 C
at its full times the world's own sensitivity, 0.6 to 1.5, less where its seas take it up: THICK AIR). Methane warms 24 C where no oxygen
burns it, 3 C where it does, and past a third of its amount an orange haze builds that cools the ground (ORANGE HAZE: tan, orange, brown
or olive by the world). Oxygen forms an ozone where the air had none (OZONE) and cools a methane air. Hydrogen sulphide yellows the
horizon (SULPHUR AIR) and with nitrous oxide thins an oxygen air's ozone (THIN OZONE). Dimethyl sulphide cools under a pale haze (COLD
HAZE) and lays a fog on the seas (SEA SMOKE). Methane an oxygen air burns turns to water high up, frozen into silver clouds in the
summer twilights of the high latitudes (SILVER CLOUDS, drawn as the noctilucent clouds' trait draws them). Ammonia warms, a cold world
most; hydrogen warms a thick air only. A world with seas, a living one's or an ocean's, whose lines take its mean over the sphere past
42 C runs away (one already over it as generated keeps its seas)
(RUNAWAY): its seas steam for six steps under a fog to 70 m, 25 C more, then dry: a living world's to their old beds (the deep basins'
salt, the slopes' dust: a scorched world's rule), an ocean's, a hycean's or an eyeball's to its floor at its heights as generated, the
salt its sea held crusting most of it, dust on the swell's rises. The runaway and the snowball's ice (below) remember: the world's mean
is walked over every line's step boundaries in order, and a runaway goes on whatever comes after (its steam keeps it, and boiled seas
are gone). What LIFT carries up: a glow into the clouds (GLOWING
CLOUDS: patches, veins or whole decks by how much, in the line's own glow), a dye into the clouds and the rain (RED CLOUDS; BLOOD RAIN
for a red or crimson, YELLOW RAIN and the rest), ice nuclei (FROST) that make every cloud rain, or snow on a cold world (ENDLESS RAIN),
spores (SPORE CLOUDS: gold, rust or white). Each look is 0 below the share its trait is named at.

## The made climate (J-04)

The layer's warmth (`MadeWorld::warmC`) and pressure (`pressureK`) are the world's now. On every kind they
move the caps' edge (1.6 degrees of latitude a degree: a thin air's, an ocean's floes, a steam world's polar seas, an eyeball's pupil, a
dry ice world's caps), the snow and frost lines (154 m a degree) and the frost fields; on a living world the ice and the frozen lakes and
rivers too, and an endless rain on a cold world snows it over. A living world's ice sheets keep their domes: where the warmth moved the
caps, the snow line and the ice's edge off one, its snow melts to bare ice, and its margin melts dark (B-493: the dark zone of a warm
summer's Greenland, out to where the ice is 8% of its dome a degree thick, 80% at most), so the white ice draws back; a glacier's tongue
lifted over 2 C melts back to its bed. A
biome moves only across the ice's edge (a greened world's follow its warmth or its rain, below), and the ground's heights and rivers stay
the world as generated (the reefs and the alpine belt read the cap and the snow line as generated). Its heat dries the plants toward straw
from 5 C over, all of them at 18 C or in a runaway (`madeMatColour`), and kills up to seven trees in ten on the ground. The HUD's TEMP
takes the warmth where there is air and ATM the pressure; the data sheet's AIR line says what moved ("THICKER, 14 C WARMER THAN IT
WAS"). A vial's warmth takes it too, where there is air, and a spread's band of warmth is read at it. A world's seedings are run in
order, each on the world the ones before it left (its place, medium, natives and share at their warmth, covers and frozen or dried
seas); the world's mean as generated stays what a runaway and the ice pass.

## The made waters, ground and plants (J-05)

What a line makes of what it lives in (`looksOf`), in four classes, each with a colour of
its own: the waters (J-03's tint), the land, the snow and ice, the plants (a world's grass and forest where it has plants). A class's look
lies where any line that makes one lives, each line's in its own colour (its waters', its land's, snow's and plants' looks and its glow
are its spread's): where several lie, a line whose look shows a colour wins over one whose does not (a dye over a line that only glows),
then the one of the greater share there, the later on a tie. The waters: a line that never stops (STOP cut)
blooms into a soup (SOUP SEAS: a light-feeder's pigment, else a murky brown), with slime a jelly whose waves come slow and heavy (JELLY
SEAS); a chalk shell turns them milky (MILK SEAS); a rust shell turns its fresh waters orange (RUST RIVERS); lift with slime piles foam on
their shores (FOAM SEAS: white, yellow or pink). A dye's colour wins over a milk's or a rust's. The land: a line of the waters lays mats
on its shores and flats within 8 m over the water (MATS), stromatolites where it slimes on a chalk shell (STROMATOLITES); a line of the
land (DRY, or of snow and ice) lays sulphur crusts, chalk, glass sands that glint, rust, crusts where its cells bind (CRUSTS: in
polygons, ripples or rosettes, each drawn in its own texture, a world's own crust beside it in its own: B-487) or slime on the rocks
(SLIME); a stone rotter pits the rock (PITTED
STONE). The snow: a dye colours a line's snow (RED SNOW and the rest), a dark one
darkens and melts its ice (DARK ICE, 1.5 C warmer). The plants: a host of plants (HOST) lights the forests at night with its glow
(GLOWING FORESTS; a glowing line's other plants' looks, a blight or a frost burn, stay dark: `plantsGlow`), gives them its dye (RED
LEAVES and the rest), burns them with its frost where the mean is under 10 C (FROST BURN), and swells
them with galls where it never stops or passes its genes on (GALLS); a wood rotter blights them in spots, stands or all by how fast it
rots (BLIGHT: grey, black or rust), and past 0.6 fells the forests (ROT: trees dead, logs down). The air turns the world's own colours
everywhere: a sulphide's waters purple, black with no sun (PURPLE SEAS, BLACK SEAS, as a black dye's), a sour sea's reefs bleached (SOUR
SEAS, on a world with reefs), an oxygen's rust over an oxygenless world's rock, sand, dust and mud (RUST GROUND), the plants lush under a
new ozone or an endless rain (LUSH: their colours deepened, their trees, bushes and fallen logs up to 1.6 times as many) and bleached
under a thin one (BLEACHED); a runaway's seas boil while they steam (BOILING SEAS). A
chalk sea cools the world 1.5 C at its full. A trait is named only where its look can show, and a cover lies at a share over a half on
land alone; no height moves.

## The animals' fates (J-08)

What the lines do to a world's animals (`MadeFate`: the traits named, the harm to the land's, the air's
and the waters' animals, to the shelled alone and to the plant eaters alone, what killed the most, a sickness and the share of the
survivors that show it, the share of each herd born changed and what steers how, thriving, the steps since it began). A line's fate
(`lineFateOf`, in its spread, from its second step to its tenth) comes from its genes where it lives: a working poison (a nerve poison
crazes them, a gut poison wastes them, a blood poison kills); a host of animals by its depth (on the skin its glow or its dye, else bare
patches; in the gut wasting; in the blood or the cells by its other genes: bloated with a gas, growths where it never stops, stone skin
with a shell, melting with a flesh rotter, sprouting with spores and binding cells, else it kills); a carrier with a host, its young born
changed; a flesh rotter festers wounds and rots remains fast; a shell rotter softens the shelled; a soup's or a jelly's waters starve
their swimmers; a wood rotter's blight the plant eaters. Its reach: the waters' animals where it lives in the sea (half in fresh waters,
a fifth in brine), the land's where it lives on the land (0.8 by the fresh waters they drink, 0.3 else), the air's half of the most of
those; a host every animal it meets. The air's fate (`madeFaunaOf`, over the whole world): a sulphide air kills (its purple seas their
swimmers first), a thinned ozone changes the young, the heat past 6 C warmer kills (a runaway every one), the cold past 4 C colder where
the world ends under 15 C (all of it at 5 C), an
ammonia sea its swimmers; a new ozone, an endless rain over the plants or a richer oxygen air makes them thrive. A fate names its traits
past their shares (a die-off 0.15 of a class, a sickness 0.15, changed young 0.1, thriving 0.2 where nothing died) and keeps nothing under
them; silence is every class dead to the last over the whole world, of the air's harm and the lines' that reached all of it. A point's
fate (`madeFateAt`) joins the air's and each line's by its spread's share there, the harms as chances, its age counted from the step the
spread reached it. The data sheet's MADE line names the animals' traits last: DIE-OFF, WASTING, MELTING, GROWTHS, SPROUTING, BLOATED,
STONE SKIN, CRAZED, CHANGED YOUNG, GLOWING ANIMALS, a dyed hide by its dye (RED HIDES), THRIVING, SILENCE. The surface turns a point's
fate into each species' (`06-surface.md`): a herd's, a flock's and a shoal's members sick and changed alike where the line meets their class
(B-490: a sprouting flyer falls dead from the air and a sprouting swimmer washes up dead), their remains staged by the landing's clock,
and a kill's or a fall's kept by the guide for 64 steps (B-489).

## The overgrowth (J-09)

A line that never stops (its STOP cut) and binds its cells grows a tissue over the land it lives on; one that
slimes without binding lays films on the wet ground by its waters (`MadeGrowth`, a spread's, from `growthOf`). Its type comes from its genes
and how it lives: a chalk shell's is CORAL, a glass shell's SPONGE, lift's BLADDERS, a flagellum's HAIR, a light-feeder that lives dried
out under a shield LICHEN, another light-feeder CARPETS, a dead-eater that rests as spores FUNGUS, a grazer that moves SLIME MOULD, else
FLESH. Its looks come from its genes too: its colour (its dye's, a light-feeder's pigment, else one of its type's own), its texture by its
cell's shape (blistered spheres, fibrous rods, folded curved cells, coiled spirals), its grain by its cell's size, its wetness by its
slime, its hardness by its shell (a dried-out one's leathery), its glow (veins and spots at night), cilia where it moves. It begins at the
line's eighth step and creeps out behind its spread (`madeReachOfStep` of its own steps), the whole world by the line's 32nd. A point's
share (`madeGrowthAt`) has a ragged edge, lobes a world's fortieth across and fingers of a few hundred metres, fixed to the place: the
edge only moves out, so what it covered stays covered. Its cover, laid after J-05's pass where the share passes a half on the land its line
lives on by its warmth (never water, snow, ice or lava; films only by a shore), is one of three materials, TISSUE, THREADS (a fungus's,
hair's, a carpet's) or HARD GROWTH (a coral's, a sponge's, a lichen's, a plated or bony one's): `madeGrowthCover`. A forest keeps its
density under it, so its trees stand wrapped; the other plants go. One overgrowth's colour shows a world, the latest growing line's
(`growthLine`). MADE names the types after the plants (FLESH, FUNGUS, SLIME MOULD and the rest). The ground's structures are the
surface's (`06-surface.md`).

## Ruins overgrown, and the sound (J-10)

The overgrowth counts its steps on past 24, to 64 (`MadeGrowth::steps`), and a point knows
how many steps it has lain there since its edge passed (`madeGrowthSince`: the first step whose reach, read with the edge's lobes and
fingers, stood past the point, and the fraction by how far past). On the ground its cover climbs each ruin from the foot by them, faster
as it goes (a house's walls in about six steps, a tower's in twelve, a capital's tallest in twenty-five), each wall by its side toward
the way the cover came; it grows shut the doorways, windows, gates and arches it climbs past (`06-surface.md`). A stone rotter's line (its
land look PITTED STONE) keeps its own steps (`MadeSpread::pits`), and the buildings it reached where it lives (within its band of
warmth) crumble over twelve of them. On the ground the ruins follow the steps as they pass. A
wet overgrowth (0.3 and over) is WET GROUND (it squelches underfoot), a flesh's or a sponge's BREATHING (its vents breathe): MADE names
them after the types. Where every animal is gone (SILENCE) nothing living is heard.

## Made kinds (J-11)

Once a world's change passes a line it takes a made kind (`MadeWorld::kind`, `madeKindOf` in `life/seeding`), the
first that holds, in this order: MADE HOTHOUSE (a runaway's seas dried), MADE SNOWBALL (its ice run to the equator), GREENED DESERT (J-15,
below), an overgrown world (an
overgrowth that reached the whole world and can lie on half its land at least, `MadeGrowth::land`; named by its type: FLESH WORLD, CORAL
WORLD; films make none), CHARNEL WORLD (every animal gone and the plants felled, dried or blighted over the world, or none), PURPLE WORLD
(purple seas under a sulphur air), RUST WORLD (an oxygenless world's ground rusted under its new ozone), HAZE WORLD (a thick orange haze),
each look past 0.6, and GREENED WORLD (a world under 10 C as generated made 2 C warmer, or a steppe's dry one rained on, where nothing was
harmed: its plants lush, its animals thriving, its biomes moved by its warmth or its rain in the biome table alone). The snowball has a
run of its own beside the runaway's: a living wet world its lines cool under -6 C from over it (Budyko's ice-albedo line) ices for six
steps (FREEZING OVER), the ice's white cooling it up to 20 C more, then is frozen over; its seas then take no fog or colour, and its sea
ice is the frozen felisian's blue, in the grass family's bank (`matFamilyOnGen`). It thaws only once its gases warm it past the ice's own
cooling too (-6 C and up to 20 C more). A made hothouse and a made snowball lay a dead world's cover over their own living
ground (`MadeWorld::deadCover`, `deadCoverOf`: the scorched felisian's, the frozen felisian's), in the dead kind's colours for the sand's
and the snow's families, with the land's heights and the rivers' beds as generated and the old forests standing dead (`06-surface.md`).
The data sheet's kind line reads "FLESH WORLD, ONCE FELISIAN", the made kind's sentence takes the kind's and `describeBody` its phrase
(`madeKindName`, `madeKindSentence`, `madeKindPhrase` in `galaxy/made`); an ocean world's hothouse names no forests. Every screen names
the kind now (`Game::kindNameNow`): the analyzer and the telescope its short name ("FLESH", `madeKindShort`), IN ORBIT, the target's
label and a screenshot's caption its name, the ORBIT and LANDING log lines both ("FLESH WORLD, ONCE FELISIAN"), the tree's world pages
too. `madeHarms` and `madeLivable` read whether a layer harmed its world's life or made it more livable (thriving, lush, a new ozone,
GREENED or a greened desert, or a mean moved 5 C or more toward 15 C, where nothing was harmed); `seed kinds` prints the shares (about half
harm, a fifth change the looks alone, a fifth make a world more livable).

## Greening a desert (J-15)

On a desert, or any bare world with air, a light-feeder's carpet or lichen grows grass and shrubs where it lies (`MadeGrowth::greens`,
set in `madeWorldOf`). Such a world is `madeGreens`: landable, with air, no plants and no seas of its own (B-482: a desert, a rust, a
salt, a thin-aired world, a scorched felisian and the rest; J-15 had the desert alone). The plants begin where the carpet's edge passed
four steps before (`MADE_GREEN_AFTER`) and stand
thick from twelve (`MADE_GREEN_THICK`): two reaches behind its edge (`greenReach`, `thickReach`) read with the edge's own lobes and
fingers (`madeGreenAt`), so the grass follows the carpet out at its pace. The planet function's last pass lays GRASS there, its density
0.32 rising to 0.65, over any land look (a crust lies under it). Their colour is the line's pigment's, a dye's over it
(`MadeGrowth::plants`; its carpet takes it too), in its line's own overgrowth bank in every view (B-482: the ground, the flora, the
globe's stain bank, each map's line ramp): green from a green pigment, purple from a purple one. Two lines of different colours each
show their own where their grass lies, the later's where both do. On the ground the flora draws them as a steppe's: tufts, and shrubs a
shade darker than the grass; no trees, flowers or berries (`06-surface.md`). The land green (`MadeWorld::green`: the green reach's share
of the sphere times the share of the land the carpet can lie on) names GRASS AND SHRUBS on the data sheet's MADE line, and GREENED
DESERT past a half, before an overgrown world ("A CARPET OF CELLS HAS GROWN INTO GRASS AND SHRUBS OVER MOST OF ITS BARE LAND."; a
lichen's names its lichen). A greening carpet harms nothing; the desert counts as more livable unless the line harms it otherwise (a
soup in its springs does, as anywhere).

A carpet is a light-feeder's with DRY, its STOP cut, its BIND set and no SHELL or LIFT (on most bare worlds the line refuses for the
cold or the ultraviolet unless tuned for them): a chalk shell makes coral, glass a sponge, lift
bladders, flagella hair, none of which green. With a SHIELD as well it is a lichen, which greens too. The gene editor says which: with
STOP cut its line reads "CUT OUT: OVERGROWS AS CARPETS, GREENS A DESERT" (`overgrowthTypeOf`, the rule the layer uses). `seed kinds`
lets a green light-feeder tuned for the first desert of the forty go there: its place is grass from its 12th step, the desert a GREENED
DESERT from its 27th.

## The record

The guide keeps each seeding as a `seeded` line: the world, the place, the line's key and a copy of its edits (a strain's
number is the guide's own), the flask's medium, the generation, and the guide's clock when it went down. The guide's clock (`clock`)
counts the real seconds the game has run, across saves and expeditions; the warp never hurries it. With them, `madeseen` keeps what each
world showed at its last landing. They are written only once a world is seeded (the clock also once an animal is kept dead, B-489), so a
guide without either saves as before. A loan
carries none of them. The game works out a world's seedings once (`Game::seedingsAt`: each line read from its key as a flask reads it),
again only when the guide's `seeded` lines change.

**Natives died out (J-12).** At its place a line beats some natives: it outgrows them, eats them or poisons them (`SeedVerdict::beaten`
and `beatenHow`). Once its spread covers the world, at its 24th step, those that live only in niches and salts the line lives in die out
(`seedingDiedOut`): each water the native lives in must hold a salt among the line's waters, and any other niche needs a line that lives
on the land; and none of its own band of warmth may lie outside the line's where the world has that warmth, its waters' for a water
niche and its land's for any other (B-474: the world's range read over the habitat's 96 points, `SeedVerdict::wetLoC` and the rest). A
later line meets none of the natives the lines before it drove out by the time it goes down (`seedVerdictOf`'s `gone`). From then no
vial of the world holds them, wherever it is taken. A native the line beat that also lives where the line
cannot does not die out: where the line spread, a vial holds a hundredth of what it held.

**Found wild (J-12).** A line seen under the microscope in a vial of the world it went down on is found wild: the guide keeps a `wild` line
(`<world>/L<k>`, the world's k-th seeding) and SEEDED in the log says so. When the sequencer reads it from such a vial the record is marked
read (the save's `seqwildkey`), and the KIN line says what the wild changed in it.

**The catalogue and the LIFE page (J-12).** A seeded world's head in the catalogue names its made kind. A native seen that died out reads
DIED OUT, and its sheet says which line did it and how (`DIED OUT: POISONED BY` and the line's words). The lines found wild follow the
world's species, each SEEDED. A line's sheet is its species' sheet with the line's form as the wild left it, when it went down, and
what the wild changed once the sequencer read it there: UNCHANGED IN THE WILD, CHANGED IN THE WILD: GENES 23, 29, else NOT YET SEQUENCED
FROM THE WILD. The animals seen on any world come last, with their marks: GONE, DYING, CHANGED, those the ground showed and (B-500) those
their world's record holds at the guide's clock, the fate of each species over its world (`animalFates`: 128 points, where it lives). An
animal's sheet draws it as the ground does (its body posed by `poseAnimal` and `emitAnimal`, its world's two hides, a scale bar in metres)
beside its way, what it was seen doing, the nearest it came and the most seen at once, its fate. By kin the animals follow their world's
genesis once one of its own species was read, else those not yet sequenced. A species gone from its world is
drawn dim. The LIFE page adds WORLDS SEEDED (and how many lines took), MADE (the made kinds now, each name once) and DIED OUT (the
explorer's species seen that died out, the catalogue's rows, and the animals seen gone), worked out as the page opens
(`catalogueCountsMake`). A friend's lent sheet of a native that died out is marked, never counted.

**Auto-explore** never seeds. It lands on a seeded world as on any other: nothing it decides reads the layer but the map it chooses its
landing by.

## What the explorer is told

The data sheet's SEEDED lines (the latest three: the line, how long ago, DIED where it did), MADE (what the
layer shows: its waters' traits, then, J-05, its ground's and its plants', then its air's, J-08 its animals' last) and, J-04, AIR (what
the made climate moved: `THICKER, HAZY, 6 C COLDER THAN IT WAS`). The LANDING log on a seeded world says what it shows that the last
landing did not ("UNKNOWN SINCE THE LAST LANDING: RED SEAS"). A vial keeps the guide's clock at its take (`vialclock`): taken where a line
had spread by then, it holds the line (a tenth of its crowd) beside the world's species, and the natives it outgrew there are a hundredth
of what they were (none once they died out). The microscope shows it (under its own key), the incubator puts it in a flask as it went
down, and the sequencer reads it with its changes in the wild. J-12: the catalogue and the LIFE page (above).

## Where it is read

Each frame the game gives the worlds of the system their layers (`Game::madeTick`, nothing in a game with no
seeding). The renderer keeps them by the world's seed (`SpaceRenderer::setMade`). A new key drops the world's generator, and the world's
map and the telescope's plates, keyed by the layer as by the season, are made again; so are the landing map, its zoom and the drone bay's
map. A landing gives the ground its world's layer (`SurfaceSite::made`), kept until the next landing. The planet function's last pass
draws the cover and the tint (`05-planet-function.md`). The globe draws the tint in a bank of its own, the maps through the tint's own
ramp, and the ground in a bank the world leaves free (`02-rendering.md`). The air's greenhouse (`BodyGen::airWarmthC`) and whether a
world has air
(`BodyGen::hasAir`) are read through the generator. J-04: the sky pair (`typeSky`) and the cloud pattern take the made air, so the
globe, the maps and the ground agree; the living ground reads the made warmth (`felisianSample`); the plants' colours come through
`madeMatColour`. J-05: the made pass gives each point the share of its class's look (`madeClassOf`) and the line whose look lies there
(`SurfaceSample::madeLine`, kept in the planet map's texels, the plates' cells and the ground's vertices), the maps a made ramp for every
material and line, the globe a disc's made colours each a bank (`SpaceRenderer::discMade`: its stain, its waters, each class's look and
its clouds, by line: 25, 27, 29, 21, 23 and 31 on the first disc, 26, 28, 30, 22 and 24 on the second), and the ground a bank each from
those the world leaves free (the lent four, 24 and 20 where nobody built, 13, 12, the aurorae's 11 and 21 where none can show), the
overgrowth first, then the waters' tint, the hides, the looks, the foam and a dyed rain, each line's own where its colour differs; its
trees, undergrowth, structures, covered rocks and taken animals in the bank of the line where they stand (`02-rendering.md`).

## The harness

`seed test` (J-02) draws a desert world's landing map, zoom, globe, telescope plate and ground as generated, with a test
tint, with a test cover, and as generated again, the last pictures the first pixel for pixel. `seed worlds` prints the six lines' verdicts
on forty worlds, their drifts, the spread's steps and what each shows. `seed sea` lets the dye line go in a sea off a coast and draws the
map, the zoom, the globe and the ground before and at its eighth step, and a glowing line's sea at night. J-04: `seed climate` prints each
gas on twenty worlds, their mean warmth, pressure and air before and after; `seed traits` draws each made air's trait from orbit and on
the ground, before and after. J-05: `seed looks` draws each made water's, ground's and plants' trait on the landing map, from orbit and on
the ground, before and after. J-08: `seed fates` lets each fate's line go where it brings it, prints what each species came to and draws
each fate, each change and a herd over its steps. J-09: `overgrowth plates` draws each overgrowth type and forty variants at a forest's
edge, and `overgrowth world` a world overgrown at four of its steps (`08-testing-and-tools.md`); J-10: `overgrowth ruins` a city and a
capital overgrown at four steps, a city crumbled, and a town at night under a glowing cover. J-11: `seed kinds` prints the made kind
each of forty worlds and six lines ends as and
the shares of harm, looks and more livable, and draws each made kind from orbit, on the data sheet and on the ground. J-12: `seed record`
lets a world's own sea line given a nerve poison go on a world with animals and draws its record at the 64th step: the catalogue by world,
a native's sheet that died out, the line's found wild, an animal's gone, the LIFE page (`record/`). J-13: `seed cold` prints forty worlds'
warmth on the ground and down, a line tuned for each, and draws the seed page 5 km down and a europan world's cracks reddened from under its
ice (`cold/`). J-14: `seed clouds` prints the cloud decks' heights, a cloud line tuned for each and water life refused, and draws a venusian
world's clouds dyed and glowing, an acid world's, and the seed page in the clouds (`clouds/`). J-15: `seed kinds` greens a desert by its
steps and draws it (`kinds/desert_*.png`). The unit's `seeding` group holds the rest
(`unit-checks.md`), and the flow seeds a world in its last stage.
