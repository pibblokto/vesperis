# Generations

`GEN_VERSION` (`src/galaxy/system.h`) is bumped once per series whenever a generator changes what a star, a system or a
surface looks like (M9-17); saves and guides record it. The table gives each generation in a line, newest first; below it,
what each bump changed, as it was noted beside the constant until E-02 moved it here word for word (the line had grown to
5,860 characters). A new bump adds its row and its section at the top, beside its entry in the release's log (`log/`).

| Gen | Date | By | What changed | Its entry |
| --- | --- | --- | --- | --- |
| 16 | 2026-10-21 | P-01..P-07, Q-01, Q-02 | 1.4.0's made kinds of world (P) and the six trait slots (Q) | [P-01](log/6-1.4.0.md) and after |
| 15 | 2026-10-18 | B-428 | a solid world's moons spread out (the first at 5-40 radii) | [B-428](log/5-1.3.2.md) |
| 14 | 2026-10-18 | B-425 | the companion stars keep clear of the other sun's worlds | [B-425](log/5-1.3.2.md) |
| 13 | 2026-10-08 | A-01 | the ages: every people has one; its end, ruins and signals follow | [A-01](log/3-1.3.0.md) |
| 12 | 2026-10-07 | X-03 | each giant's character on its globe | [X-03](log/2-1.2.0.md) |
| 11 | 2026-10-01 | G-01 | the galaxy of 200 billion stars: every star's existence, class and name | [G-01](log/1-1.1.0.md) |
| 10 | 2026-09-28 | O6-03..O6-06 | drainage, the landform library, the landmarks | [O6-03..O6-07](log/0-1.0.0.md) |
| 9 | 2026-09-28 | B-322 | the ocean worlds' floes on a shelf sea, banks over their water | [O11](log/0-1.0.0.md) |
| 8 | 2026-09-28 | B-321, B-320, R-304, R-305 | the felisian greenhouse, rivers and lakes rebuilt, traits and landforms | [O10](log/0-1.0.0.md) |
| 7 | 2026-09-28 | B-314 | the first moon at 4.5 radii and outside the rings | [the table](log/0-1.0.0.md) |
| 6 | 2026-09-27 | O6-02 | the relief spectrum | [the table](log/0-1.0.0.md) |
| 5 | 2026-09-27 | O0-01 | rings on 12% of solid planets | [the table](log/0-1.0.0.md) |
| 4 | 2026-09-26 | N5-03 | the gas giants' storm term | [the table](log/0-1.0.0.md) |
| 3 | 2026-09-26 | M5 | astronomy: companion stars, substellar objects, comets, Kepler orbits, double planets | [the table](log/0-1.0.0.md) |
| 2 | 2026-09-26 | M9 | plates, rivers, biomes, seasons, four new types | [the table](log/0-1.0.0.md) |
| 1 | before 2026-09-26 | iteration 1 | the first worlds | |

## 16

P-01 the furnace worlds, the first of 1.4.0's made kinds (the kinds of 1.4.0 are this one bump): `pickPlanetType` draws the old
kinds as before and every draw of the system is made as the drawn kind's, then `kindOf` makes about one body in fifty a magma
ocean, a chthonian, a glass, a steam, a salt or a rust world by hashes of their own (never a world with a people): those bodies'
kinds, sizes, masses, colours and surfaces change, a made chthonian's rings, and the few moons that would lie within three of a
made world's radii move out; every other body, every orbit and every star do not; P-02 the living worlds (archean, hothouse,
superhabitable, steppe, tundra, lichen: about one body in twenty-four made with the furnace worlds, the home world kept
felisian); P-03 the locked and the deep (eyeball, terminator, hycean, ammonia, snowball, chemotroph: about one body in fourteen
made in all; the companions and the comets read the moons' orbits as drawn, so no star moves); P-04 the dead worlds (the
scorched felisian from a giant's felisian worlds and a white dwarf's rocky and desert planets, its peoples by a hash of their
own; the frozen felisian made in no system yet); P-05 the far cold (nitrogen, cryovolcanic, dry ice, brine dwarf and frost dwarf
from the icy, cratered, europan and thin-air worlds of the cold orbits, a giant's captured moons made now too; and the
wanderers: one system in twenty-seven caught a rogue world, an extra last body drawn from a stream of its own, so every other
body keeps its index and its draws; about one body in ten made in all); P-06 the strange moons (grooved ice, two-faced,
tiger-striped, patchwork and sponge, made from moons by what they are among their planet's moons, read from the draws: no moon
moves, a moon has none; about one body in nine made in all); P-07 the giants (the super-puff from a young star's giants, the
water-cloud and the azure from the warm drawn giants, and the giants of the warm zones from the worlds there: the water-cloud,
the azure, the alkali and the silicate giants of Sudarsky's classes and the gas dwarf, the hot ones only from worlds locked to
their star; the moons a made giant reaches move out, never a people's: a giant that would move one is not made; about one body
in eight made in all); Q-01 the six trait slots (three new slots by hashes of their own, R-304's three as drawn; eleven
landforms in them on about three worlds in five, never a people's or the home world, whose ground stands as it was) and the
landforms' own moisture and warmth off the living ground (an acidic world's karst stands: the dune seas, canyons, badlands, salt
flats, glaciers and patterned ground of the wet and the cold-aired worlds follow); Q-02 the water, the sky, the life and the
ground (sixteen traits in the new slots by kind, the landforms' share falling; those that move the ground or take a bank never
on a people's world or the home world, whose sky and sea may glow: the tides' flats, the hot springs, the reefs, the gypsum's
dunes, the strata, the meteorites' pits and the waters' colours change the surfaces that carry them; a body's tidal forcing a
derived fact, no draw)

## 15

B-428 a solid world's moons spread out (its first moon at 5-40 radii, 4.5-6 before; a twin at 12-20, 5-9 before): their orbits,
periods and so a locked moon's day change, and where a world's reach grew its companion's floor and its comets can; giants'
moons, every surface and the stars do not

## 14

B-425 the companion stars keep clear of the other sun's worlds (a far one moves out until it spans 3 degrees at most of their
sky, its light alone warms them to 100 K at most and its own worlds keep within a third of the gap; a close one bright beside
the primary or wider than it is far instead): the companions' orbits and, where one moved, its own worlds' warmth change; every
primary's worlds and the stars do not

## 13

A-01 the ages (the civilisation expansion of 1.3.0 is this one bump): every people has an age, its end's years are its age's
(longer for the late ages) and its ruins weather by them, only the peoples that had radio transmit (the transmitter cells' salt
and rate re-pinned, so the signals move) and the charts' heard marks follow; systems, stars and every surface do not

## 12

X-03 the giants' character on the globe (each giant's share of storms, an ice giant's faint bands, a great storm on one giant in
four: the gas giants' and brown dwarfs' globes change; systems, stars and every solid surface do not)

## 11

G-01 the galaxy of 200 billion stars (`galaxyDensity`: every star's existence, class and name can change; the whole G/S series
of 1.1.0 is this one bump)

## 10

O6-03..O6-06 drainage (rivers routed downhill by a flood of the ground, lakes at the fill levels of real basins, valleys carved
to the rivers), the landform library (cliff bands, broken plains, scree, coasts by exposure, cirques and fjords, lava tongues,
slip-face dunes, crater terraces) and the landmarks (heights, water and materials change on every solid surface; systems and
star positions do not)

## 9

B-322 the ocean worlds' floes rise from a shelf sea through an ice foot, river and lake banks stand over their plane (heights
change on ocean worlds and along felisian water; systems do not)

## 8

B-321 the felisian greenhouse (+33 K over the equilibrium temperature: biomes, ice caps and water move on every living world),
B-320 rivers and lakes rebuilt, R-304/R-305 planet traits and landforms (every solid surface changes; systems and star positions
do not)

## 7

B-314 the first moon at 4.5 radii and outside the rings (moon orbits and periods change; surfaces do not)

## 6

O6-02 the relief spectrum (every solid surface changes shape; systems and star positions do not)

## 5

O0-01 rings on 12% of solid planets

## 4

N5-03 gas giant storm term

## 3

M5 astronomy
