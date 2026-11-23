# Rules: a seeded world's made kind

Rules for the made kind a seeded world takes once its change passes a line (1.5.0, R-448: J-11 on), one line per rule with its log
entries (`log/`); a replaced rule moves to `superseded.md`. What the lines make of the air, waters, ground, plants and animals is in
`rules-life-made.md`, the overgrowth in `rules-life-overgrowth.md`.

## The kinds
- A world takes the first made kind whose line it passed (`madeKindOf`, life/seeding), none before; a world nobody seeded has none [J-11]
- MADE HOTHOUSE: a runaway's seas dried; MADE SNOWBALL: its ice run to the equator; then an overgrowth over the world and half its land [J-11]
- An overgrown world is named by its type (FLESH WORLD, CORAL WORLD and the rest); films lie by the waters alone and make none [J-11]
- CHARNEL WORLD: every animal gone (SILENCE) and the plants felled, dried or blighted all over, or none; it comes before the looks' kinds [J-11]
- PURPLE WORLD, RUST WORLD, HAZE WORLD: purple seas under a sulphur air, rust under a new ozone, an orange haze, each look past 0.6 [J-11]
- GREENED WORLD: a cool world (under 10 C as generated) made 2 C warmer, or a dry one rained on, where nothing was harmed [J-11], [B-455]
- GREENED DESERT: a bare world with air, half its land a carpet's or a lichen's grass and shrubs, before an overgrown world; harming nothing [J-15], [B-482]
- A greened world's plants are lush, its animals thrive and its biomes follow its warmth or its rain; its heights and rivers stay [J-11], [B-452]
- A made heat dries plants and kills animals by how warm the world now is too (none under 5 C, all from 15 C): a cold world warmed greens [J-11]
- A made cold kills animals by how cold the world now is too (none over 15 C, all from 5 C): a hot world cooled to the temperate is spared [B-454]
- A kind remembers: a runaway goes on whatever comes after; a snowball thaws only once its gases pass its ice's own cooling too [B-453]
- A living wet world cooled under -6 C from over it ices for six steps (FREEZING OVER), up to 20 C colder, then is frozen over [J-11]
- Its frozen seas take no fog and no colour, and their traits are not named; a made hothouse's rain dries before it reaches the ground [J-11]
- A made snowball's sea ice is the frozen felisian's blue, in the grass bank (`matFamilyOnGen`) [B-452]
- An ocean world's made hothouse lies on its dry floor, salt and dust at its heights as generated; its sentence names no forests [B-450]

## Drawn and told
- A made hothouse lays the scorched felisian's cover over its own living ground, a made snowball the frozen felisian's (`deadCoverOf`) [J-11]
- Either lies over the world as generated (no made climate under it): the land's heights and rivers' beds as they were, the old forests dead [J-11]
- Their sand's and snow's families take the dead kind's colours in every view (`madeMatColour`); rock, water and plants keep the world's [J-11]
- The data sheet's kind line reads "FLESH WORLD, ONCE FELISIAN"; the made kind's sentence takes the kind's, `describeBody` its phrase [J-11]
- Every screen names the kind now: the analyzer and the telescope its short name, IN ORBIT and the target its name, the log both [B-447]
- Harm, looks and more livable are read from the layer (`madeHarms`, `madeLivable`); `seed kinds` prints their shares over six lines [J-11]
- More livable: thriving, lush, a new ozone, a greened world or desert, or a mean moved 5 C or more toward 15 C, where nothing was harmed [B-455], [J-15]

[B-447]: log/7-1.5.0.md
[B-450]: log/7-1.5.0.md
[B-452]: log/7-1.5.0.md
[B-453]: log/7-1.5.0.md
[B-454]: log/7-1.5.0.md
[B-455]: log/7-1.5.0.md
[J-11]: log/7-1.5.0.md
[J-15]: log/7-1.5.0.md
[B-482]: log/7-1.5.0.md
