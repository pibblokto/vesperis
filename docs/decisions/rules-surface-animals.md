# Rules: the animals

Rules for a world's animals (1.5.0, R-448: J-06 on), one line per rule with its log entries (`log/`); a replaced rule moves to
`superseded.md`. Where herds live (the habitat cells) is in `rules-surface-site.md`.

## The bestiary
- A world's stream sets only the counts: 3 to 5 species on land where there are plants, 1 or 2 in the air, 2 to 4 in the water [J-06]
- Each species comes from its own seed (the world's, land, air or water, its place among them): a new draw moves no other species [J-06]
- A species' ecology and its body draw on two streams of their own; a new value draws after every existing draw of its stream [J-06]
- One giant a world at most; a floater only in an air of 1.3 atmospheres or more; a glider only where there are trees [J-06]
- The herds' cells and spots read only whether a world has land animals: the unit holds generation 17's digest of them [J-06]

## Bodies
- A body is its plan's recipe of parts (`bodyOf`), never a plan's own drawing code: the same rig and pose draw all sixteen [J-06]
- A world's style (`AnimalStyle`) is kept by a species only where it fits its plan (a weight of 1 or more): no beaked quadrupeds [J-06]
- No two of a world alike: a body is drawn again, 24 times at most, while it is under `ANIMAL_ALIKE` of one kept before it [J-06]
- Animals are named by the explorer: the generated names go with J-07's record (the plan's default 12) [J-06]

## Drawing
- A rig is built once a species at the landing; each frame poses it, and a pose reads only its inputs [J-06]
- The main thread reads the ground under each animal (3 by 3 heights) and draws its shadow; poses and shapes are made on the pool [J-06]
- Each animal's shapes go in a list of its own; the lists are filled in bands in their order, so any thread count gives one frame [J-06]
- Detail by the animal's height in pixels: lines far away, a coarse mesh, the whole near; no joint, tail or wing under the ground [J-06]
- Hides take two banks of the world's own: 24 and 20 where no people built, 13, 12, then a free lent one; else the forest's and rock's [J-06]
- Horns, tusks, claws and beaks take the bone bank 29, hooves and mandibles the dark 30, eyes and insect wings the snow's 8 [J-06]
- The plates (`bestiary plates`) are how bodies are judged: the user's verdict on them comes before the next milestone [J-06]

[J-06]: log/7-1.5.0.md
