# Rules: the animals

Rules for a world's animals (1.5.0, R-448: J-06 on), one line per rule with its log entries (`log/`); a replaced rule moves to
`superseded.md`. Where herds live (the habitat cells) is in `rules-surface-site.md`.

## The bestiary
- A world's stream sets only the counts: 3 to 5 species on land where there are plants, 1 or 2 in the air, 2 to 4 in the water [J-06]
- Each species comes from its own seed (the world's, land, air or water, its place among them): a new draw moves no other species [J-06]
- A species' ecology and its body draw on two streams of their own; a new value draws after every existing draw of its stream [J-06]
- One giant a world at most; a floater only in an air of 1.3 atmospheres or more; a glider only where there are trees [J-06]
- The herds' cells and spots read only whether a world has land animals: the unit holds generation 17's digest of them [J-06]
- A species' way and how many live together are its ecology stream's last draws; a hunter's prey is chosen from its world's others [J-07]

## Bodies
- A body is its plan's recipe of parts (`bodyOf`), never a plan's own drawing code: the same rig and pose draw all sixteen [J-06]
- A world's style (`AnimalStyle`) is kept by a species only where it fits its plan (a weight of 1 or more): no beaked quadrupeds [J-06]
- No two of a world alike: a body is drawn again, 24 times at most, while it is under `ANIMAL_ALIKE` of one kept before it [J-06]
- Animals have no generated name: the explorer names one with Shift+N, the crosshair on it (N is the map); unnamed, it goes by its words [J-06], [J-07] [R-452]
- No two of a world read alike: a species' words add the first part of its look none of its world's others of its kind share [J-07]

## Drawing
- A rig is built once a species at the landing; each frame poses it, and a pose reads only its inputs [J-06]
- The main thread reads the ground under each animal (3 by 3 heights) and draws its shadow; poses and shapes are made on the pool [J-06]
- Each animal's shapes go in a list of its own; the lists are filled in bands in their order, so any thread count gives one frame [J-06]
- Detail by the animal's height in pixels: lines far away, a coarse mesh, the whole near; no joint, tail or wing under the ground [J-06]
- Hides take two banks of the world's own: 24 and 20 where no people built, 13, 12, a free lent one, the aurorae's; else forest's and rock's [J-06], [B-456]
- A climber clings to the trunk as drawn (its girth, taper and lean), its body along its axis, its feet on the bark's round, clear of others [B-468], [B-491]
- A climber takes the nearest trunk within 25 m as thick as its body is wide at the most it climbs to; with none near, it climbs none [B-491]
- A swimmer of the shallows swims a stretch of its cell's water 0.9 m deep, found on the 16 m ground and the mean water [B-467]
- Each step of a shallows' stretch follows that depth where the last point lies, its chord within the 0.3 to 1.5 m it swims in [B-492]
- Horns, tusks, claws and beaks take the bone bank 29, hooves and mandibles the dark 30, eyes and insect wings the snow's 8 [J-06]
- The plates (`bestiary plates`) are how bodies are judged: the user's verdict on them comes before the next milestone [J-06]

## Ways and the record
- A gait's stride follows its speed (`speciesStride`); a foot on the ground stays where it was put while the body goes over it [J-07]
- The ways are states of the one machine (`updateLife`): a way's own state is left only by its own end, never by the generic choice [J-07]
- A hunt has one try at the kill within a body's length, by a chance from 0.15 to 0.6 by size; the quarry's remains feed the pack [J-08]
- Seen is drawn whole (a coarse mesh or more) on the screen within reach; the record reads `drawLife`'s, never the update's [J-07]
- The guide keeps a species seen by `<body key>/A<index>` (`animal` lines, written once there is one); a loan carries only the names [J-07]

## Fates
- A line's fate comes of its genes where it lives (its steps 2 to 10), the air's of the made air over the whole world (`madeFaunaOf`) [J-08]
- A fate keeps nothing under the share its trait is named at; no thriving where any die-off is named; silence is every class gone [J-08]
- A species' fate at a place is the layer's there (`madeFateAt`) by how it resists, a hash of its seed; past 0.95 harm none is spared [J-08]
- A herd, a flock and a shoal lose their dead; their sick and changed by each member's hash, where the line meets their class [J-08], [B-469]
- A flock's or a shoal's changed young can circle or limp, a flyer's stumble or fly on after dark; a crazed flyer falls to the ground at times [B-490]
- A species' changes are its own on its world: one to three its body takes, the first steered by the lines' genes, none clashing [J-08]
- Remains are a pure function of their cell, read from the planet function, never the drawn ground; old bones lie on any world [J-08]
- Remains age by their fate's age where they lie and the landing's clock since, three times as fast with a flesh rotter [J-08], [B-470]
- The guide keeps a world's kills and falls 64 steps (`dead` lines): a cell met again, at any landing or load, has them dead where they fell [B-470], [B-489]
- A sprouting one dies 4 to 16 steps after its fate came (by its hash): on its high ground, from the air, washed up from the water [B-469], [B-490]
- A scavenger feeds at remains where no hunter feeds; a hungry hunter feeds at remains within 120 m before it hunts [J-08]
- Remains seen record DYING (GONE where none of it lives there), a changed young CHANGED; the guide keeps the marks; old bones mark none [J-08]
- Remains are drawn a few centimetres toward the eye, so the ground they lie on never hides them [J-08]

[B-456]: log/7-1.5.0.md
[B-467]: log/7-1.5.0.md
[B-468]: log/7-1.5.0.md
[B-469]: log/7-1.5.0.md
[B-470]: log/7-1.5.0.md
[B-489]: log/7-1.5.0.md
[B-490]: log/7-1.5.0.md
[B-491]: log/7-1.5.0.md
[B-492]: log/7-1.5.0.md
[J-06]: log/7-1.5.0.md
[J-07]: log/7-1.5.0.md
[J-08]: log/7-1.5.0.md
[R-452]: log/7-1.5.0.md
