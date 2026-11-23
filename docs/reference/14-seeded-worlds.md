# Seeded worlds

A culture grown aboard and sent down by a drone changes the world it lives on (R-448, 1.5.0's J series; the design
`docs/ideas/IDEAS-seeded-worlds.md`). J-01 gave every species fourteen world genes (`13-life.md`, "The genes"). J-02 made the layer a
seeded world is drawn through, J-03 sends a culture down and shows the first change, J-04 changes the air and the climate, J-05 the
waters, ground and plants. J-06 and J-07 drew the animals and their ways (`06-surface.md`), J-08 their fates. J-09 grows the overgrowth,
J-10 lets it take the ruins and gives it sounds, J-11 names the made kinds, J-12 keeps the record. A world nobody seeded has no layer and
is drawn as generated, bit for bit. The rules: `../decisions/rules-life-world.md` (the genes and the layer), `rules-life-seeding.md`
(sending and the record), `rules-life-made.md`, `rules-life-overgrowth.md` and `rules-life-kinds.md`.

<!-- contents -->
Contents:
- Sending a culture down
- Whether it takes
- The drift
- The layer
- The made air (J-04)
- The made climate (J-04)
- The made waters, ground and plants (J-05)
- The animals' fates (J-08)
- The overgrowth (J-09)
- Ruins overgrown, and the sound (J-10)
- Made kinds (J-11)
- The record
- What the explorer is told
- Where it is read
- The harness
<!-- /contents -->

## Sending a culture down

Parked in orbit of a landable world, S on the drone bay opens the seed page (the state `SEEDING`): the
world's map with the ground under the ship and the places of its lines, the culture (C: the next flask that shows FULL, at its crowd or
overgrown), the place (the arrows, two degrees a press, ten with Shift) and what lies there, read at the world map's scale (`seedPlaceOf`:
the sea, a lake, the sea shore, snow, ice, a salt crust, soil, sand, bare rock and the rest, as a vial there would be named). Enter sends
drone 1 alone. It falls for about sixteen seconds, lets the culture go as it arrives (the guide's `seeded` line, SEEDED in the log),
watches two minutes and climbs; its report is the verdict ("UNKNOWN: CULTURE GROWS", "CULTURE DIED, TOO COLD"). The flask keeps its
culture, and the bay charges as after any trip. A save keeps a trip in flight (`bayseed`). Refused: at home (the pinned home star's first
felisian planet, where every game starts), at a giant, out of orbit, with the drones away or charging, and where three lines live.

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

## The drift

A line that takes can change in the wild: one or two of its genes (never a vital one) stepped a letter or broken, by a
hash of the world's seed, the line's key and the place, in about a third of seedings, twice as often under a strong ultraviolet. The
spread and its looks are the changed line's. The sequencer reading the line from a vial of that world says CHANGED IN THE WILD: GENE n.

## The layer

`madeWorldOf` gives a world's layer from its seedings at the guide's clock: the first three that took, each a spread round
its place, 2 km at step 1, 150 km at step 6, the whole world at step 24, a step each five minutes at the line's pace (its doubling there
against two hours', half to one and a half, times one plus its LIFT). Within its reach a spread keeps to where its line lives: the waters
of its salt (as `growthIn` passes them), the land where it lives dried out (or in snow and ice), its band of mean warmth (`placeMeanC`).
A dye colours the waters a line lives in, a rust of the dye's colour mixed into the world's water (the latest dye's, from the first step);
a glowing line lights them at night. The data sheet names the traits ("RED SEAS", "GLOWING SEAS", or "WATERS" on a world without seas).
A layer that draws nothing (lines that only spread, or died) has key 0, so no map is made again for it. The layer's key hashes every field.

## The made air (J-04)

What the lines that took give a world's air (`madeAirOf`, with the layer): each gas by its gene's amount, times
the share of the world its line lives on over 0.15 (`SeedVerdict::share`, read at 96 points over the sphere), rising from the line's
step 6 to its step 24, so the air changes after the looks. The world answers by what it is, a gas leaving an airless one. CO2 warms (20 C
at its full times the world's own sensitivity, 0.6 to 1.5, less where its seas take it up: THICK AIR). Methane warms 24 C where no oxygen
burns it, 3 C where it does, and past a third of its amount an orange haze builds that cools the ground (ORANGE HAZE: tan, orange, brown
or olive by the world). Oxygen forms an ozone where the air had none (OZONE) and cools a methane air. Hydrogen sulphide yellows the
horizon (SULPHUR AIR) and with nitrous oxide thins an oxygen air's ozone (THIN OZONE). Dimethyl sulphide cools under a pale haze (COLD
HAZE) and lays a fog on the seas (SEA SMOKE). Ammonia warms, a cold world most; hydrogen warms a thick air only. A living world with seas
whose mean over the sphere passes 42 C runs away (RUNAWAY): its seas steam for six steps under a fog to 70 m, 25 C more, then dry to
their old beds (the deep basins' salt, the slopes' dust: a scorched world's rule). What LIFT carries up: a glow into the clouds (GLOWING
CLOUDS: patches, veins or whole decks by how much, in the line's own glow), a dye into the clouds and the rain (RED CLOUDS; BLOOD RAIN
for a red or crimson, YELLOW RAIN and the rest), ice nuclei (FROST) that make every cloud rain, or snow on a cold world (ENDLESS RAIN),
spores (SPORE CLOUDS: gold, rust or white). Each look is 0 below the share its trait is named at.

## The made climate (J-04)

The layer's warmth (`MadeWorld::warmC`) and pressure (`pressureK`) are the world's now. On a living world
they move the caps' edge (1.6 degrees of latitude a degree), the snow line (154 m a degree), the ice and the frozen lakes and rivers; an
endless rain on a cold world snows it over. A biome moves only across the ice's edge, and the ground's heights and rivers stay the
world as generated (the reefs and the alpine belt read the cap and the snow line as generated). Its heat dries the plants toward straw
from 5 C over, all of them at 18 C or in a runaway (`madeMatColour`), and kills up to seven trees in ten on the ground. The HUD's TEMP
takes the warmth where there is air and ATM the pressure; the data sheet's AIR line says what moved ("THICKER, 14 C WARMER THAN IT
WAS"). Samples, a later line's verdict and the spreads' bands read the warmth as generated.

## The made waters, ground and plants (J-05)

What a line makes of what it lives in (`looksOf`), in four classes, each with a colour of
its own: the waters (J-03's tint), the land, the snow and ice, the plants (a world's grass and forest where it has plants). A class's look
is the latest line's that makes one, and it lies where any line that makes one lives. The waters: a line that never stops (STOP cut)
blooms into a soup (SOUP SEAS: a light-feeder's pigment, else a murky brown), with slime a jelly whose waves come slow and heavy (JELLY
SEAS); a chalk shell turns them milky (MILK SEAS); a rust shell turns its fresh waters orange (RUST RIVERS); lift with slime piles foam on
their shores (FOAM SEAS: white, yellow or pink). A dye's colour wins over a milk's or a rust's. The land: a line of the waters lays mats
on its shores and flats within 8 m over the water (MATS), stromatolites where it slimes on a chalk shell (STROMATOLITES); a line of the
land (DRY, or of snow and ice) lays sulphur crusts, chalk, glass sands that glint, rust, crusts where its cells bind (CRUSTS) or slime on
the rocks (SLIME); a stone rotter pits the rock (PITTED STONE). The snow: a dye colours a line's snow (RED SNOW and the rest), a dark one
darkens and melts its ice (DARK ICE, 1.5 C warmer). The plants: a host of plants (HOST) lights the forests at night with its glow (GLOWING
FORESTS), gives them its dye (RED LEAVES and the rest), burns them with its frost where the mean is under 10 C (FROST BURN), and swells
them with galls where it never stops or passes its genes on (GALLS); a wood rotter blights them in spots, stands or all by how fast it
rots (BLIGHT: grey, black or rust), and past 0.6 fells the forests (ROT: trees dead, logs down). The air turns the world's own colours
everywhere: a sulphide's waters purple, black with no sun (PURPLE SEAS, BLACK SEAS, as a black dye's), a sour sea's reefs bleached (SOUR
SEAS, on a world with reefs), an oxygen's rust over an oxygenless world's rock, sand, dust and mud (RUST GROUND), the plants lush under a
new ozone or an endless rain (LUSH) and bleached under a thin one (BLEACHED); a runaway's seas boil while they steam (BOILING SEAS). A
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
swimmers first), a thinned ozone changes the young, the heat past 6 C warmer kills (a runaway every one), the cold past 4 C colder, an
ammonia sea its swimmers; a new ozone, an endless rain over the plants or a richer oxygen air makes them thrive. A fate names its traits
past their shares (a die-off 0.15 of a class, a sickness 0.15, changed young 0.1, thriving 0.2 where nothing died) and keeps nothing under
them; silence is every class dead to the last over the whole world, of the air's harm and the lines' that reached all of it. A point's
fate (`madeFateAt`) joins the air's and each line's by its spread's share there, the harms as chances, its age counted from the step the
spread reached it. The data sheet's MADE line names the animals' traits last: DIE-OFF, WASTING, MELTING, GROWTHS, SPROUTING, BLOATED,
STONE SKIN, CRAZED, CHANGED YOUNG, GLOWING ANIMALS, a dyed hide by its dye (RED HIDES), THRIVING, SILENCE. The surface turns a point's
fate into each species' (`06-surface.md`).

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
as it goes (a house's walls in about six steps, a tower's in twelve, a capital's tallest in twenty-five; `06-surface.md`). A stone rotter's
line (its land look PITTED STONE) keeps its own steps (`MadeSpread::pits`), and the settlements it reached crumble over twelve of them. A
wet overgrowth (0.3 and over) is WET GROUND (it squelches underfoot), a flesh's or a sponge's BREATHING (its vents breathe): MADE names
them after the types. Where every animal is gone (SILENCE) nothing living is heard.

## Made kinds (J-11)

Once a world's change passes a line it takes a made kind (`MadeWorld::kind`, `madeKindOf` in `life/seeding`), the
first that holds, in this order: MADE HOTHOUSE (a runaway's seas dried), MADE SNOWBALL (its ice run to the equator), an overgrown world (an
overgrowth that reached the whole world and can lie on half its land at least, `MadeGrowth::land`; named by its type: FLESH WORLD, CORAL
WORLD; films make none), CHARNEL WORLD (every animal gone and the plants felled, dried or blighted over the world, or none), PURPLE WORLD
(purple seas under a sulphur air), RUST WORLD (an oxygenless world's ground rusted under its new ozone), HAZE WORLD (a thick orange haze),
each look past 0.6, and GREENED WORLD (a world under 5 C as generated made 3 C warmer, or a steppe's dry one rained on, where nothing was
harmed: its plants lush, its animals thriving). The snowball has a run of its own beside the runaway's: a living wet world its lines cool
under -6 C from over it (Budyko's ice-albedo line) ices for six steps (FREEZING OVER), the ice's white cooling it up to 20 C more, then is
frozen over; its seas then take no fog or colour. A made hothouse and a made snowball lay a dead world's cover over their own living
ground (`MadeWorld::deadCover`, `deadCoverOf`: the scorched felisian's, the frozen felisian's), in the dead kind's colours for the sand's
and the snow's families, with the land's heights and the rivers' beds as generated and the old forests standing dead (`06-surface.md`).
The data sheet's kind line reads "FLESH WORLD, ONCE FELISIAN", the made kind's sentence takes the kind's and `describeBody` its phrase
(`madeKindName`, `madeKindSentence`, `madeKindPhrase` in `galaxy/made`). `madeHarms` and `madeLivable` read whether a layer harmed its
world's life or made it more livable; `seed kinds` prints the shares.

## The record

The guide keeps each seeding as a `seeded` line: the world, the place, the line's key and a copy of its edits (a strain's
number is the guide's own), the flask's medium, the generation, and the guide's clock when it went down. The guide's clock (`clock`)
counts the real seconds the game has run, across saves and expeditions; the warp never hurries it. With them, `madeseen` keeps what each
world showed at its last landing. They are written only once a world is seeded, so a guide without a seeding saves as before. A loan
carries none of them. The game works out a world's seedings once (`Game::seedingsAt`: each line read from its key as a flask reads it),
again only when the guide's `seeded` lines change.

**Natives died out (J-12).** At its place a line beats some natives: it outgrows them, eats them or poisons them (`SeedVerdict::beaten`
and `beatenHow`). Once its spread covers the world, at its 24th step, those that live only in niches and salts the line lives in die out
(`seedingDiedOut`): each water the native lives in must hold a salt among the line's waters, and any other niche needs a line that lives
on the land. From then no vial of the world holds them, wherever it is taken. A native the line beat that also lives where the line
cannot does not die out: where the line spread, a vial holds a hundredth of what it held.

**Found wild (J-12).** A line seen under the microscope in a vial of the world it went down on is found wild: the guide keeps a `wild` line
(`<world>/L<k>`, the world's k-th seeding) and SEEDED in the log says so. When the sequencer reads it from such a vial the record is marked
read (the save's `seqwildkey`), and the KIN line says what the wild changed in it.

**The catalogue and the LIFE page (J-12).** A seeded world's head in the catalogue names its made kind. A native seen that died out reads
DIED OUT, and its sheet says which line did it and how (`DIED OUT: POISONED BY` and the line's words). The lines found wild follow the
world's species, each SEEDED. A line's sheet is its species' sheet with the line's form as the wild left it, when it went down, and
what the wild changed once the sequencer read it there: UNCHANGED IN THE WILD, CHANGED IN THE WILD: GENES 23, 29, else NOT YET SEQUENCED
FROM THE WILD. The animals seen on any world come last, with the marks the ground showed: GONE, DYING, CHANGED. An animal's sheet is in
words alone: its way, what it was seen doing, the nearest it came and the most seen at once, its fate. A species gone from its world is
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
ramp, and the ground in a lent bank (`02-rendering.md`). The air's greenhouse (`BodyGen::airWarmthC`) and whether a world has air
(`BodyGen::hasAir`) are read through the generator. J-04: the sky pair (`typeSky`) and the cloud pattern take the made air, so the
globe, the maps and the ground agree; the living ground reads the made warmth (`felisianSample`); the plants' colours come through
`madeMatColour`. J-05: the made pass gives each point the share of its class's look (`madeClassOf`), the maps a made ramp for every
material, the globe its made bank to one class (`madeGlobeClass`: the waters', else the land's, the plants', the snow's), and the ground a
lent bank to each class, its trees and undergrowth in the plants' (`02-rendering.md`). J-09: an overgrowth's cover is its own colour on
the maps; the globe gives its made bank to the stain first (the first disc's waters' tint then takes bank 31), and the ground lends it a
bank before the classes' looks.

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
capital overgrown at four steps and a city crumbled. J-11: `seed kinds` prints the made kind each of forty worlds and six lines ends as and
the shares of harm, looks and more livable, and draws each made kind from orbit, on the data sheet and on the ground. J-12: `seed record`
lets a world's own sea line given a nerve poison go on a world with animals and draws its record at the 64th step: the catalogue by world,
a native's sheet that died out, the line's found wild, an animal's gone, the LIFE page (`record/`). The unit's `seeding` group holds the
rest (`unit-checks.md`), and the flow seeds a world in its last stage.
