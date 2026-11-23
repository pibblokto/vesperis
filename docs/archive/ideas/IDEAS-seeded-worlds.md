# Ideas: seeded worlds (cultures sent down, worlds changed, animals extended)

The user's request of 2026-11-20 (R-448), for 1.5.0: the J series in `ITERATION-1.5.0.md`, Phase 6. Nothing here is built. This note
holds the design and the lists; the iteration file holds the milestones, their checks and their order.

The asking, in short: send a gene-edited culture down to a world; if it survives it changes the world "in a different unpredictable
way, sometimes beneficial (making it livable, adding atmosphere, affecting vegetation), sometimes neutral, sometimes harmful". More
genes, "more nuanced" than a gene for thick air: "a microb with tweaked gene can produce more co2 which will transform planet into
venusian type of planet. Another gene may cause water turn red, another make luminescent clouds, another add smoke on the water". New
traits "of artificial type", some never found in the wild. Harm as bio horror: animals dying out with remains "modeled after animals
that are actually living on the planet", mutations "with ribs being seen, flesh melting", vegetation dying, "sometimes it's
statistically up horror". A world covered in flesh, with ruins overgrown and new structures, and variety inside every trait: "red flesh,
irregular flesh, etc. Or not flesh, maybe mushrooms! Or it's vegetation of different colors". And "it's time to extend animals
variation drastically. And still procedural".

This reverses one rule of 1.5.0: "the worlds stay as generated" (`IDEAS-genes.md` 1.8, the left-out list of `ITERATION-1.5.0.md`).
A world nobody seeded stays as generated, bit for bit.

## 0. What exists

Four readings of the code (2026-11-20) found:

- **Genes:** each genesis has a map of 18 places (3 vital, 3 quiet, warmth, salt, size, pigment, food, motion, 3 senses, rest, glow,
  shape), shuffled on one salt. A species has a gene for each thing it uses. The editor cuts, changes by hand, moves, copies and switches
  them; a moved or faster gene slows growth (the price). The guide keeps edits and learned genes by place number.
- **Limits no code reads yet:** every species carries an oxygen need (none, tolerated, needed), a pH range and a radiation dose. Nothing
  judges a line against a world. `growthIn` and `mediumOfPlace` judge one square metre's medium; `pairOf` judges two species in a flask.
- **Worlds:** a `Body` is regenerated from its star in about 20 places and `BodyGen::make` is called from about 30, so a change can't
  be stored on the body. Air composition does not exist; pressure, greenhouse, fog and rain come from the kind's row. The drainage's
  key holds `tempBias`, so warming the world as generated would move rivers. The dead worlds (P-04) already keep heights and rivers
  under a new cover (`thenWorld`, `nowTempC`).
- **Microbes on the ground today:** red snow (`snowTint`, bank 28), the hot springs' bands (bank 25), pink lakes, milky seas and blooms
  (`waterTint`, `lifeTint`), dead and fungal forests. All are fixed at generation.
- **Banks:** all 32 are assigned. On many fauna worlds 12, 13, 18-20, 24 and 25-28 are set but unused; 29 (bone-pale), 30 (dark) and 31
  (ochre) are set on every world. One palette serves the globe, the maps and the ground.
- **Animals:** 8 plans (6 on land, a flyer, a swimmer), drawn each frame from boxes and lines. Hides borrow a ground bank, so a forest
  animal is the forest's colour. Flyers are a point and two lines. A world has one swimmer species, seen as fins. One random stream draws
  a whole world, names are generated and shown, nothing is saved per species, and no animal is ever dead.
- **Plants:** no species. A world has two silhouettes from ten families; leaves share bank 3 with the forest floor.
- **Ruins:** `Form`s in the people's banks, one colour per piece. The rasteriser already draws a second bank over a face with a ragged
  edge (the terrain's material borders) and has a blob form ("a grown form: a lumpy body and its lobes").
- **Devices:** the drone bay (C-18) flies three drones from orbit to places on the world's map on a real-time clock; it refuses worlds
  without a people. The probe falls only into giants. The capsule is the landing.
- **Time:** the lab's clock runs in real seconds while the game simulates and resets with a new expedition. The guide is shared by every
  save and expedition, and already holds what the explorer took from worlds (shards and instruments).

## 1. Sending a culture down

The drone bay gets a second trip. Parked in orbit of a landable world, the explorer opens the drone bay, presses S, chooses a culture
and a place, and sends one drone.

```
DRONE BAY                                     READY
SEED ASHRA III
+-------------------------------------------------+
|      ...      ~~~~~~~~                          |
|   ......   +    ~~~~~~~~~~        ..            |
|    ....          ~~~~~~           ....          |
+-------------------------------------------------+
CULTURE  LIGHTUS IGNORUS E2, FLASK 3, FULL
PLACE    12 N 140 E, SEA SHORE
C CULTURE  ARROWS PLACE  ENTER SEND  ESC BACK
```

- **The culture:** a flask that shows FULL, so enough cells go down. The flask keeps its culture. A culture from the collection is
  grown in a flask first.
- **The place:** the point under the ship, moved with the arrows on the world's map. What lies there (sea, lake, river, shore, land,
  snow, ice) decides the medium.
- **The trip:** the drone falls, lets the culture go, watches two minutes and climbs. Its report says what it saw, in the incubator's
  words: "ASHRA III: CULTURE GROWS" or "ASHRA III: CULTURE DIED, TOO COLD". The bay charges as after any trip.
- **Refused:** not in orbit, a giant ("NO GROUND DOWN THERE"), the home world ("HOME STAYS AS IT IS"), a culture not at its crowd,
  drones away or charging, a fourth line on one world.
- **Several lines:** up to three a world. Each acts on the world as the ones before left it. Nothing undoes one.

## 2. Whether it takes

A pure function of the world, the line and the place (`life/seeding`), checked in this order, the first failure said:

1. **Solvent:** the line's chemistry must be the world's liquid (water, acid, ammonia, hydrocarbon). A furnace kills everything.
2. **The place's medium:** `mediumOfPlace` read at the place, judged by `growthIn`: warmth, salt, light for a light-feeder, food for a
   chemical-eater. TOO HOT, TOO COLD, WRONG SALT, NO LIGHT and NO FOOD as in the incubator.
3. **Oxygen, pH and dose:** the limits every species carries, read now for the first time. The place's oxygen comes from the air, its
   pH from the kind and the place (an acid world, a soda lake, a spring), its dose from the field and the air (more on an airless world
   with no field). SHIELD raises the dose a line can take.
4. **Land:** on land beyond the water a line needs DRY.
5. **The natives:** where the world has life of the line's niche at the place, the line must outgrow it (`pairOf` with the price paid).
   A heavily edited line pays more and can lose. POISON kills natives it meets and gives it an edge.

A line that takes can **change in the wild:** one or two of its genes stepped or broken, by a hash of the world, the line and the place,
more often under a strong ultraviolet. The explorer finds out by sampling the world and reading the line on the sequencer ("CHANGED IN
THE WILD: GENE 29"). A STOP broken in the wild is how a world can end up overgrown when no one meant it.

The same seeding always gives the same world, so reloading a save can't buy a better outcome. A different line, place or world gives a
different one.

## 3. Time and the spread

- **The clock:** the guide's own clock of seconds the game simulated, kept in the guide across saves and expeditions. It runs while
  the game runs, wherever the ship is; the warp does not hurry it.
- **The spread:** closed-form on that clock, from the place outward through the parts of the world the line can live in: the band of
  warmth that suits it (by latitude and height), the waters it takes (sea, fresh, brine), land with DRY, the air with LIFT. The place
  changes within about 5 minutes, the region around it (a landing map's view) within about 30, the whole world within about 2 hours. A
  slow grower takes longer, a line with LIFT less.
- **Steps:** the spread is read in 24 steps over its course, as the seasons are, so the maps and the globe are made again only when the
  step changes.
- **What changes when:** looks first (stains, tints, glow), then the air and the climate, then life (plants, then animals), then the
  overgrowth. A world's made kind (section 10) comes at the end.

## 4. World genes

Fourteen new genes, at places 19 to 32 of every genesis's map. Places 1 to 18 are drawn exactly as now, so every learned gene, strain
and edit in a guide reads the same. Every species carries all fourteen; in most species most of them do nothing (a table reading NONE,
a weight of 0). So one organism can be edited into every variation by hand (the user's call for R-443), and a kin that already has a
gene working saves the hand work. They follow the editor's rules: cut, changed by hand, moved in from kin, copied, switched, and priced.

Each gene is a thing a cell makes or does. None of them says what it does to a world: the world decides.

| Gene | Code | Wild | Seen in the lab |
| --- | --- | --- | --- |
| GAS | a table (NONE, CO2, METHANE, OXYGEN, HYDROGEN SULPHIDE, DIMETHYL SULPHIDE, NITROUS OXIDE, AMMONIA, HYDROGEN), a weight (how much) | by how it lives and what it burns | bubbles; the gas named once seen against a parent |
| DYE | a table (NONE, RED, CRIMSON, ORANGE, YELLOW, VIOLET, BLUE, BLACK, FLUORESCENT), a weight | the red snow's alga, the springs' bands | the flask's water coloured |
| LIFT | a weight: gas vesicles, cells that float | open-water species, the blooms' | a scum at the top |
| FROST | a weight: how warm it freezes water | snow and cloud species | freezes at a warmer setting |
| SLIME | a weight: a coat of slime | mats and springs | thick medium, a coat in the drop |
| SHELL | a table (NONE, CHALK, GLASS, RUST, SULPHUR), a weight | magnet-turners RUST, reef seas CHALK | a sediment |
| STOP | a weight: the crowd it stops at | every species, at its crowd | cut or NEVER: OVERGROWN, the flask fills |
| BIND | a weight: cells stick after dividing | colonies | clumps in the drop; with STOP off, tissue |
| ROT | a table (NONE, WOOD, FLESH, SHELL, STONE), a weight | dead-eaters WOOD | on a world only |
| POISON | a table (NONE, NERVE, BLOOD, GUT), a weight | hunters | a neighbour in a mixed flask dies |
| CARRIER | a weight: how readily the line's genes pass to other microbes | one species in four | a neighbour in a mixed flask takes one of the line's genes |
| HOST | a table (NONE, PLANTS, ANIMALS, BOTH), a weight: skin, gut, blood, cells | NONE | on a world only |
| DRY | a weight: how long it lives dried out | land species | on a world only |
| SHIELD | a weight: ultraviolet it can take | open ground under a strong sun | grows under the incubator's new UV light |

- **Lengths:** a table gene has 2 letters of choice and 4 of weight (6); a weight-only gene 4.
- **The price:** a working GAS, DYE, SLIME or SHELL costs growth by its weight (a few percent), as a glow does today. A line loaded with
  products grows slower, so it can lose to a world's natives. A switch works as today: GAS only IN THE LIGHT, STOP NEVER WHEN CROWDED.
- **Learning:** as Y-02, by an edit seen against its parent ("FLASK 3: GIVES OFF METHANE, PARENT CO2", then "GENE 23: GAS"). ROT, HOST
  and DRY show only on a world: a world becomes the experiment ("WHERE IT SPREAD THE TREES ROT: GENE 24 OR 27").
- **The incubator** shows the new looks in each flask, on its screen and in the room: bubbles, coloured water, scum, frost, sediment, a
  flask full of tissue pressing on the glass. An overgrown flask stays overgrown until emptied.

```
GENE EDITOR                         WORLD GENES 2/2
LIGHTUS IGNORUS E2, FROM FLASK 3
19 GAS: METHANE    =      26 NOT KNOWN
20 NOT KNOWN              27 NOTHING SEEN
21 DYE: RED        =      28 NOT KNOWN
22 NOT KNOWN              29 STOP            -
23 NOTHING SEEN           30 NOT KNOWN
24 NOT KNOWN              31 NOT KNOWN
25 NOT KNOWN              32 NOT KNOWN
GENE 29, 1,410 LETTERS: STOP
CUT OUT
PGUP PAGE  ARROWS GENE  ENTER LETTERS  C CUT
```

## 5. How a world answers

A world's answer depends on what it is: the air (none, thin, as ours, thick; oxygen or not), the warmth (how near freezing, how near a
runaway), the waters, iron in the rock or not, life (the stage, plants, animals, whether the natives need oxygen), the magnetic field and
the sun's ultraviolet. Three hashed values make worlds of one kind answer differently: how much a gas warms it, how much its seas take up,
and how each native species resists.

The same gene does different things on different worlds:

- **GAS CO2:** warms (THICK AIR). A wet world that passes about 45 C runs away: its seas steam, then dry to their old beds (RUNAWAY,
  then MADE HOTHOUSE). A frozen world thaws: ice draws back, seas open, rain comes (GREENED if life can use it). Seas turn sour and reefs
  bleach (SOUR SEAS).
- **GAS METHANE:** warms more. With little oxygen in the air an orange haze builds over the hours and then cools the ground (ORANGE
  HAZE, HAZE WORLD). In an oxygen air it burns away: a little warming only.
- **GAS OXYGEN:** on a world without oxygen, iron ground rusts (RUST GROUND), an ozone forms (OZONE, then LUSH where plants live), and
  native species that can't bear oxygen die out. The methane in the air goes with them, so a cold world can freeze over (MADE SNOWBALL:
  one of Earth's oldest ice ages may have begun this way). On a world that has oxygen, more makes the crawlers' young grow big.
- **GAS HYDROGEN SULPHIDE:** seas turn purple (PURPLE SEAS), the air yellows at the horizon (SULPHUR AIR), animals die (DIE-OFF:
  one proposed cause of the end-Permian extinction), the ozone thins.
- **GAS DIMETHYL SULPHIDE:** fog lies on the seas (SEA SMOKE) and a haze cools the world (COLD HAZE).
- **GAS NITROUS OXIDE:** the ozone thins (THIN OZONE): leaves bleach, animals' young are born changed.
- **GAS AMMONIA:** warms a cold world; in water, seas turn green and swimmers die.
- **GAS HYDROGEN:** warms a thick air; in a thin one it escapes and nothing happens.
- **DYE:** stains the waters it lives in (RED SEAS and the rest, by its colour), snow (COLOURED SNOW) and crusts. A dark dye on ice melts
  it and warms the world a little (DARK ICE). With LIFT it colours clouds and rain (COLOURED CLOUDS, BLOOD RAIN); with HOST it colours
  leaves or animals.
- **LIFT:** scums and foam (with SLIME: FOAM SEAS). It carries the line's other genes into the air: GLOW makes GLOWING CLOUDS, DYE
  coloured clouds, FROST rain, REST SPORE CLOUDS, POISON a poisoned rain far from the seas.
- **FROST:** with LIFT, clouds rain (ENDLESS RAIN): a dry world greens, a cold one snows over. With HOST PLANTS, leaves blacken (FROST
  BURN).
- **SLIME:** MATS on shores and flats, slime on rocks, foam; JELLY SEAS when STOP is off in the sea; STROMATOLITES with CHALK.
- **SHELL:** CHALK turns seas milky (MILK SEAS), whitens the ground and slowly cools the world; GLASS makes glittering sands; RUST
  rusts rivers; SULPHUR leaves yellow crusts. With BIND and STOP off: CORAL or SPONGE.
- **STOP off:** a bloom that never ends (SOUP SEAS: the natives starve and die out). With BIND: the overgrowth (section 7). With HOST
  ANIMALS: tumours.
- **ROT:** WOOD rots forests (BLIGHT, ROT); FLESH, with HOST ANIMALS, melts hides, and remains rot to bones fast; SHELL softens shelled
  animals until they die; STONE pits rock (PITTED STONE), turns rivers to acid with RUST or SULPHUR (RUST RIVERS) and crumbles ruins.
- **POISON:** NERVE makes animals circle and fall (CRAZED, then DIE-OFF); BLOOD kills and leaves dark stains; GUT wastes the grazers. In
  water the swimmers die and wash up.
- **CARRIER:** its genes pass into the world's own microbes, which then glow, stain or give off gas wherever they live, past the line's
  own range. With HOST they pass into plants (GALLS, NEW LEAVES, GLOWING FORESTS) and into animals' young (CHANGED YOUNG).
- **HOST:** PLANTS: leaves take its DYE, GLOW, FROST, STOP off (galls) or ROT (blight from inside). ANIMALS, by depth: on the skin it
  glows or colours or bares patches; in the gut it wastes them; in the blood it bloats them (GAS), grows tumours (STOP off), turns hides
  to stone (SHELL), melts them (ROT FLESH) or sprouts stalks from them (REST with BIND); in the cells, with CARRIER, it changes their young.
- **The genes of today** add their own: a light-feeder's PIGMENT colours the carpets and new leaves, GLOW lights seas, clouds, forests,
  animals and veins in tissue, REST gives spores and fruiting bodies, MOTION makes slime moulds crawl and hair wave, SHAPE and SIZE set
  the texture and grain of tissue, WARMTH and SALT decide where the line lives.

**Harm is common.** Of seedings that take, about half harm the world's life, about a third change only how it looks, and about one in
five makes it more livable. These shares come out of the rules above and the natives' resistance, and a harness mode prints them over
many worlds and lines so they can be set. A sterile world has no life to harm: there a seeding changes looks or makes it more livable.

## 6. Made traits

What a seeding makes, as the data sheet's MADE line names them (the explorer still names worlds and species). Each trait has variants:
the same trait looks different on two worlds.

| Kind | Traits | Variants |
| --- | --- | --- |
| Air (7) | THICK AIR, RUNAWAY, ORANGE HAZE, SULPHUR AIR, OZONE, THIN OZONE, COLD HAZE | how far it went; the haze tan, orange, brown or olive |
| Clouds and rain (6) | GLOWING CLOUDS, COLOURED CLOUDS, BLOOD RAIN, SEA SMOKE, ENDLESS RAIN, SPORE CLOUDS | glow blue, green or amber; patches, veins or whole decks; spores gold, rust or white |
| Water (11) | RED SEAS (by the dye's colour), PURPLE SEAS, BLACK SEAS, MILK SEAS, GLOWING SEAS, FOAM SEAS, JELLY SEAS, SOUP SEAS, SOUR SEAS, RUST RIVERS, BOILING SEAS | streaks, shores or whole seas; glowing surf or whole seas; foam white, yellow or pink |
| Ground (11) | COLOURED SNOW, DARK ICE, CRUSTS, MATS, STROMATOLITES, RUST GROUND, CHALK GROUND, GLASS SANDS, SULPHUR CRUSTS, PITTED STONE, SLIME | colours and bands; crusts in polygons, ripples or rosettes |
| Plants (8) | BLIGHT, ROT, FROST BURN, GALLS, NEW LEAVES (by colour), GLOWING FORESTS, LUSH, BLEACHED | spots, stands or all; grey, black or rust; galls on trunks or limbs |
| Animals (12) | DIE-OFF, WASTING, MELTING, GROWTHS, SPROUTING, BLOATED, STONE SKIN, CRAZED, CHANGED YOUNG, GLOWING ANIMALS, THRIVING, SILENCE | which species, how many of a herd, which change (section 8) |
| Overgrowth (10) | FLESH, FUNGUS, SLIME MOULD, LICHEN, CORAL, SPONGE, HAIR, BLADDERS, CARPETS, FILMS | thousands (section 7) |
| Sound (2) | WET GROUND, BREATHING | the squelch, the breath's pace |

The ones a wild world never shows: RUNAWAY in progress, BLOOD RAIN, JELLY SEAS, SOUP SEAS, all of the overgrowth but lichen, and most
of the animals' fates.

```
ASHRA III
FELISIAN
TRAITS      ARCHIPELAGO, FOG SEAS
SEEDED      LIGHTUS IGNORUS E2, 2 H AGO
MADE        RED SEAS, ORANGE HAZE, DIE-OFF
AIR         THICKER, 9 C WARMER THAN IT WAS
```

## 7. The overgrowth

A line with BIND, and with STOP off, keeps growing past the crowd. The cells stick together and grow into a tissue that covers the
ground. The kind of tissue depends on how the line lives and on its genes:

- **FLESH:** a chemical-eater, or a hunter (a hunter's flesh takes in what dies on it).
- **FUNGUS:** a dead-eater that rests: threads over everything and fruiting bodies (caps, puffballs, shelves, cages, cups).
- **SLIME MOULD:** a grazer that moves: yellow or orange veins that pulse and creep, with small fruiting stalks.
- **LICHEN:** a light-feeder with DRY and SHIELD: crusts and hanging beards on rock, trees and walls.
- **CORAL:** CHALK: branching spires, fans and domes on land. **SPONGE:** GLASS: porous vases and towers.
- **HAIR:** a line whose MOTION gives flagella: fields of fine filaments that wave in the wind.
- **BLADDERS:** LIFT: gas sacs on stems; some break free and drift up into the sky.
- **CARPETS:** a light-feeder: a new vegetation of fronds, reeds and rosettes in the colour of its pigment (violet, black, red, yellow).
- **FILMS:** SLIME without BIND: sheets over water and wet ground that ripple and bubble.

**Variety inside a type**, from the line's genes:

- colour: its pigment and DYE (red, pink, pale, bruised grey-blue, purple, brown, black, clear with dark veins);
- texture: SHAPE (spheres blistered, rods fibrous like muscle, curved folded like a brain, spirals coiled like gut);
- grain: SIZE (smooth or knobbly);
- wetness: SLIME (dry and leathery, glistening, dripping);
- hardness: SHELL (soft, leathery, plated, bony);
- light: GLOW (veins or spots lit at night);
- hair: MOTION (bare or furred with cilia).

**On the ground:** a cover with a ragged creeping edge, its own detail tile for each texture (veins, pores, folds, threads). Trees and
rocks under it are wrapped or swallowed; remains on it are taken in, bones showing.

**Structures** stand on the cover, drawn and walked round like rocks: mounds; ridges that run across the ground like roots or blood
vessels; blisters and sacs; stalks with bulbs; folds; vents that open and breathe out vapour; rib arches and rows of teeth (with CHALK, in
bone's colour); membranes stretched between stalks; polyps that sway. Fungus adds caps and shelves, slime mould creeping veins, coral
spires, sponge vases, hair waving fields and bladders rising sacs.

**Ruins:** the walls are taken over from the foot up, the edge climbing as the spread goes on; doorways and windows grow shut with
membranes the explorer can push through; nodules and drapes hang from walls and roofs; floors inside are covered. Graves and works too.

**From orbit:** the land changes colour as the tissue spreads, a stain creeping out from the place; on the night side lit veins glow.

**Sound:** steps squelch, vents breathe slowly, the tissue creaks; hair fields hiss in the wind.

## 8. The animals' fates

**Die-offs.** A share of each species dies, by the cause: POISON, hydrogen sulphide, thin ozone, ROT FLESH, food lost to blight, cold or
heat. Remains are the species' own bodies in death:

- **fresh:** lying on the side, legs stiff, neck bent back (the pose fossils show), the hide dark; flies over it;
- **bloated:** the body swollen;
- **sunken:** the hide sunk over the ribs and hips;
- **bones:** skull, spine, ribs and legs, bleached (bank 29);
- **scattered:** bones apart and half buried.

They lie where the herds lived and thickest at water, where the sick go. Fliers lie with wings spread, swimmers float and wash up. A
surviving hunter feeds at them. ROT FLESH moves them through the stages fast. A wild world has a few old bones too, so remains alone don't
always mean a seeding.

**Sickness:** WASTING (thin, ribs showing, slow), MELTING (sagging hide, sores, drips), GROWTHS (tumours), SPROUTING (stalks from the
body; the animal climbs to high ground and dies there, as ants do with Ophiocordyceps), BLOATED, CRAZED (circling, stumbling, falling),
STONE SKIN.

**Changed young** (CARRIER with HOST, or a thin ozone): a share of each herd is born changed, one to three changes from this list,
chosen by the line's genes and a hash of the species:

| Kind | Changes |
| --- | --- |
| Wasting and wounds | ribs showing, melting hide, open sores, bare patches, flayed, bloated body |
| Growths | tumours, sprouting stalks, stone plates, overgrown tusks, horn growths |
| Body | extra leg, missing leg, two heads, extra eyes, no eyes, twisted spine, joined pair, short legs, long legs, long neck, extra tail |
| Size | giant, dwarf |
| Hide | new colour, pale, dark, glowing spots, glowing stripes, new pattern, iridescent, woolly, hairless, see-through |
| Ways | calm, bold, walks at night, circles, stumbles, climbs, swarms, limps |

The genes steer the choice: SIZE gives giants or dwarfs, DYE a colour, GLOW spots or stripes, a spiral SHAPE twisted spines, STOP off
tumours and extra limbs, ROT FLESH sores, SHELL stone plates and horns, a weak NERVE poison circling.

**Thriving:** more herds, bigger herds, bigger animals, new grazing on a greened world. **Silence:** every animal gone: remains
everywhere and no calls on the wind.

## 9. The animals extended

Every world's animals are drawn again from a richer generator (generation 18). The parts:

- **Body:** 1 to 12 segments with a girth along them; long and low or short and tall.
- **Limbs:** 0 to 6 pairs (many short legs for a crawler); 2 to 4 joints; knees forward or back; stance upright, sprawling or
  columnar; hooves, pads, claws, spikes or flat feet.
- **Neck and head:** a neck long or short, arched or straight. A head with a long snout, a blunt face, a beak, a trunk, mandibles or a
  shovel. 0 to 8 eyes in a pair, a row or a ring. Jaws, tusks, horns (straight, curved, spiral, antlers, a nose horn), ears or frills.
- **Tail and back:** whip, thick, club, fan, fin, stinger or curl; plates, spines, a sail, a shell or humps.
- **Wings:** a rigid sheet with fingers, a membrane between the limbs, four insect wings, or gliding skin between the legs.
- **Hide:** fur, scales, plates, bare and wet, warts, quills or long hanging hair. Patterns: plain, stripes, spots, rosettes, bands, a
  saddle, eyespots, countershading, mottling, a stripe down the back. Two banks of their own a world, so animals stop wearing the forest's
  colour.
- **Size:** a hand to a house; one giant a world at most.
- **Plans** (16, each a recipe for the parts, not its own drawing code): quadruped, long-necked browser, hexapod, octopod, biped
  strider, hopper, giant walker, serpent, many-legged crawler, slug (a foot and a shell), burrower, climber, glider, flyer, swimmer,
  floater (a gas bag drifting in a thick air).
- **Kin:** a world's animals share a style (eye count, joints, hide, colours), so they look related and each world looks its own. No two
  species of a world look alike (a measure like the microbes' `MORPH_ALIKE`).
- **Swimmers:** several species a world in seas, lakes and rivers (fish-like, eel-like, ray-like, long-necked), seen in shallows and
  breaking the surface.
- **Ways:** gaits for every body (waves of legs, trot, gallop, bound, hop, slither, crawl, flap, glide, swim, drift); grazers, browsers
  reaching up, hunters stalking and chasing herds, scavengers at remains, burrowers that drop out of sight, climbers on trunks and
  cliffs, flocks and swarms, the solitary and the territorial, night walkers.
- **The record:** each species seen is kept in the guide, as microbes are. The explorer names them (N close to one); the generated names
  go, as the root rule says nothing has a name until the explorer gives one.
- **Drawn:** a mesh built once a species and posed each frame, on the pool's threads; a few lines far away, as now. Each species draws
  from its own seed, so a new value moves no other species.

## 10. Made kinds

When a world's change passes a line, the data sheet gives it a made kind. Its heights and rivers stay as generated:

- **MADE HOTHOUSE:** the seas gone to steam, thick hot air (the scorched felisian's cover over its own ground);
- **MADE SNOWBALL:** frozen from pole to pole (the frozen felisian's cover);
- **HAZE WORLD:** under an orange haze, dim and cold;
- **PURPLE WORLD:** purple seas under a sulphur air;
- **RUST WORLD:** red ground under an oxygen sky;
- **OVERGROWN WORLD:** named by its type (FLESH WORLD, FUNGUS WORLD, CORAL WORLD and the rest);
- **CHARNEL WORLD:** no animals or plants left, remains everywhere;
- **GREENED WORLD:** a cold or dry world made living: warmth, rain, plants.

```
ASHRA III
FLESH WORLD, ONCE FELISIAN
LIVING TISSUE COVERS THE LAND FROM COAST TO COAST.
SEEDED      LIGHTUS IGNORUS E2, 5 H AGO
MADE        FLESH, BREATHING, SILENCE
```

## 11. What the explorer is told

- **The drone's report** and the log (SEEDED) when it climbs; "CULTURE GROWS" or why it died.
- **The data sheet:** SEEDED (the line and how long ago) and MADE (the traits so far); AIR when the climate moved; the kind line for a
  made kind. On the ground the HUD's TEMP and ATM read the world as it is now.
- **The LANDING log** on a seeded world: what has changed since the last landing.
- **Samples** of a seeded world find the line where it spread (changed in the wild or not) and fewer of the natives it beat. The
  catalogue marks species that died out (DIED OUT) and the bestiary animals that are gone or changed.
- **The LIFE page:** worlds seeded, made kinds, species died out.
- **Loans:** a friend's seeded worlds never reach the borrower's game.
- **Auto-explore** never seeds; it lands on seeded worlds like any other.

## 12. How it plugs in

- **Genes** (`life/genes`): `GeneKind` appended (saved as numbers); places 19 to 32 drawn on a new salt after the first 18, which keep
  their draw; the new tables after `shape`, each on its own salt (a longer table would move every table after it). No new substrate
  (`CE_BROTH = SU_COUNT` is saved). A new light, ULTRAVIOLET, appended with its `LIGHT_FIT` row. The editor's map in two pages
  (`MAP_ROWS`). The unit's check that an unedited line is its species bit for bit stays.
- **The layer:** the data of what a world was made into at a step, defined in `galaxy` (`galaxy/made.h`) and computed in `life`
  (`life/seeding`) from the guide's records. The game puts it into `BodyGen` by value in `SpaceRenderer::genFor`,
  `SurfaceSite::initFrame` and the landing map's probe site, and strips it in `unseasoned` and `thenWorld` (the peoples' plans and the
  sights stay as generated). Every cache keyed by a body's seed keys the step too: the generators, the maps, the telescope's plates, the
  zoom and the drone map.
- **The planet function:** one last pass after `darkGround`, with masks that never fade (B-404) and an early exit, so the globe, the
  maps and the ground agree. The drainage stays the world as generated; the made climate is a temperature of now (as `nowTempC` is)
  that moves ice, snow and plants, never rivers. A runaway's seas dry to their old beds (`oldSeaM`, the dead deserts' rule).
- **Kind-level reads:** where a made world changes air, fog, rain or warmth (`tempOffset` on 28 lines, `site.atmosphere` on 81), the
  reads go through `BodyGen`, not the kind's row.
- **Banks:** a made colour first takes the bank of what it replaces (water, leaves, sky, clouds), then a bank the world leaves free;
  bones and dried flesh use 29 to 31. A world shows at most four made colours at once.
- **Materials:** 25 of 32 are used. The overgrowth takes three (soft tissue, threads, hard), each with a detail tile per texture.
- **Structures and remains:** per-cell lists like `forDomes` and `planCellHerd`, drawn and collided; trees and rocks skip them.
- **Ruins:** the rasteriser's second bank with weights per vertex on the forms (the terrain's ragged edges), blob forms added to a
  ruin cell's levels, and new `part` codes for the cover's bank.
- **Animals:** `Bestiary::make` per species from its own seed, the mesh once a species, `drawLife` on the pool, the fates applied after
  `Bestiary::make` (`planCellHerd`, `updateLife`, `drawLife`).
- **The record:** a guide `seeded` line (the world, the place, the line's key and a copy of its edits, since strain numbers are a
  guide's own, the medium, the generation and the clock) and the guide's `clock` line. The save holds only a seed trip in flight (the
  bay's lines). A loan carries no `seeded` line.
- **Samples:** the line found beside `lifeOf`, never under a world's `<body>/M<i>` keys (they would collide across loans).
- **The harness:** `seed` (the verdicts, the spread, frames before and after), `seed traits`, `seed fates`, `bestiary plates`,
  `overgrowth plates`; unit groups for seeding, made worlds and animals; the flow seeds a world in its last stage.

## 13. Sources

- Kasting 1988, Icarus 74: the runaway and moist greenhouse. Arney et al. 2016, Astrobiology 16: the pale orange dot (methane haze).
- Holland 2006, Phil Trans R Soc B 361: the oxygenation of the air; Kopp et al. 2005, PNAS 102: the snowball Earth set off by oxygen.
- Canfield 1998, Nature 396: the sulphidic ocean; Kump, Pavlov and Arthur 2005, Geology 33: hydrogen sulphide and mass extinction.
- Charlson, Lovelock, Andreae and Warren 1987, Nature 326: dimethyl sulphide, clouds and climate.
- Ravishankara, Daniel and Portmann 2009, Science 326: nitrous oxide and the ozone. Pierrehumbert and Gaidos 2011, ApJ Letters 734:
  hydrogen greenhouse worlds.
- Christner et al. 2008, Science 319: biological ice nucleators in snowfall. Walsby 1994, Microbiol Rev 58: gas vesicles.
- Lutz et al. 2016, Nature Communications 7: red snow darkening glaciers. Holligan et al. 1983, Nature 304: coccolithophore blooms.
- Amils et al. 2007, Planetary and Space Science 55: Rio Tinto's acid red river. Danovaro et al. 2009, PLoS ONE 4: marine mucilage.
- Williamson et al. 2006, Nat Rev Microbiol 4: prodigiosin, the blood-red pigment. Garcia-Pichel and Castenholz 1991, J Phycol 27:
  scytonemin, a sunscreen. Battista 1997, Annu Rev Microbiol 51: Deinococcus. Belnap and Lange 2001: biological soil crusts.
- Abercrombie and Heaysman 1954, Exp Cell Res 6: contact inhibition. Ratcliff et al. 2012, PNAS 109: multicellularity evolved in yeast.
  Tero et al. 2010, Science 327: slime mould networks. Ochman, Lawrence and Groisman 2000, Nature 405: lateral gene transfer.
- Chilton et al. 1977, Cell 11: crown gall, genes moved into plants. Ruby 1996, Annu Rev Microbiol 50: a squid lit by its bacteria.
- Hughes et al. 2011, BMC Ecology 11: ants killed by Ophiocordyceps. Scheele et al. 2019, Science 363: the amphibian chytrid panzootic.
  Kock et al. 2018, Science Advances 4: the saiga die-off. Hewson et al. 2014, PNAS 111: sea star wasting. Pearse and Swift 2006,
  Nature 439: the devil facial tumour. Johnson et al. 1999, Science 284: extra limbs in frogs from a parasite. Anderson, Cembella and
  Hallegraeff 2012, Annu Rev Mar Sci 4: harmful algal blooms.
- Graham et al. 1995, Nature 375: oxygen and giant insects. Payne 1965, Ecology 46: the stages of a carcass. Faux and Padian 2007,
  Paleobiology 33: the death pose.
