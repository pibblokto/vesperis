# Rules: the world genes and the seeded worlds

Rules for the genes that change worlds and for the worlds they change (1.5.0, R-448: the J series), one line per rule with its log
entries (`log/`); a replaced rule moves to `superseded.md`. A genesis's map and the gene editor are in `rules-life-genes.md`, the
incubator in `rules-life-cultures.md`.

## The world genes
- Every species carries fourteen world genes at places 19 to 32: gas, dye, lift, frost, slime, shell, stop, bind, rot, poison, carrier, host, dry, shield [J-01]
- Places 1 to 18 are drawn as Y-02 drew them; the world places and their six tables take salts of their own (the unit's digest holds it) [J-01]
- A table gene has 2 letters of choice and 4 of weight, a weight gene 4; an amount works past a quarter of its weight, as a rest's does [J-01]
- Wild values are read from what a species is (way, food, niche, form, dose), never drawn; a table reading NONE keeps a working weight [J-01]
- No wild poison works, no wild species overgrows or has a host, and a species' own world genes cost it nothing [J-01]
- Wild frost is a melt dweller's (snow, ice or cloud, at its best above 0 C): a colder one never freezes its own flask [J-01]
- A world gene moves no cell value; LIFT, SLIME and BIND show in the drop as the form's gas vesicles, coat and arrangement [J-01]
- STOP moves the crowd by twice the change of its weight's share (-1.5 to +1 decades); cut or switched off, the line never stops [J-01]
- A product its species does not make (gas, dye, slime, shell, poison) costs 2% and up to 4% more by amount; more of its own up to 4% [J-01]
- A world gene's own switch is ALWAYS; switched off, it loses its work as if cut and costs nothing [J-01]

## In the lab
- A flask shows what its world genes make (`flaskShow`): bubbles while it grows, a dye and a sediment that stay, a scum, slime, clumps [J-01]
- A line that never stops passes its crowd by two decades and stays OVERGROWN till emptied: tissue where its cells bind, else packed cells [J-01]
- A water flask freezes below its salt's freezing point at the warmth of a culture's ice (FROST); everything in it waits (FROZEN) [J-01]
- The UV light kills a line without a shield (KILLED BY UV); a shielded light-feeder takes little light from it [J-01]
- A poison one of a pair makes and the other does not kills the other once the poisoner has a hundredth of its crowd (POISONED) [J-01]
- Genes pass between two of one genesis once both grew, from the carrier to the other: shown and learned, no law changed [J-01]
- World genes are learned against the parent as Y-02's are; ROT, HOST and DRY show only on a world [J-01]
- The editor's map has two pages, cell genes 1 to 18 and world genes 19 to 32: the arrows walk across both, Page Up and Page Down turn [J-01]

## The seeded worlds' layer
- A world nobody seeded has no layer and is drawn as generated, bit for bit: every read of the layer does nothing when its key is 0 [J-02]
- The layer is data (`galaxy/made.h`), worked out in `life/seeding` as a pure function of the guide's `seeded` lines and its clock [J-02]
- The game puts it into the generator by value where the drawing gets one (`genFor`, `initFrame`, the probe site); `make` gives none [J-02]
- The peoples, the sights and the rivers read the world as generated: `unseasoned`, `thenWorld` and `buildTile` take the layer off [J-02]
- Caches key on the layer as on the season: the generators, the planet maps, the plates, the landing map, its zoom, the drone map [J-02]
- The ground takes its world's layer at the landing, and a re-anchor keeps it [J-02]
- The made pass is the planet function's last, after `darkGround`: it moves no height, and its cover never lies on water or lava [J-02], [J-03]
- A made look's share never fades with the detail (B-404's rule), so the globe, the maps and the ground draw one edge [J-02]
- A made tint has a bank of its own: the globe's 25 (the first disc) or 26, the ground's 28, else 27, where the world's traits leave it [J-02]
- The guide's clock counts the real seconds the game runs, across saves and expeditions; the warp never hurries it [J-02]
- `seeded` and `clock` lines are written once a world is seeded (a guide without them saves as before); a loan carries none [J-02]
- A seeding's step comes every five minutes of the clock at its pace, 24 in all; three lines live on a world at most [J-02], [J-03]
- The air's greenhouse and whether a world has air are read through the generator (`airWarmthC`, `hasAir`), not the kind's row [J-02]

## Sending a culture down
- S on the drone bay sends drone 1 alone with a flask that shows FULL to a place on the map; the flask keeps its culture [J-03]
- Refused at home (the pinned star's first felisian planet), a giant, out of orbit, the drones away or charging, three lines living [J-03]
- The culture is let go as the drone arrives (its `seeded` line then); called back before that, nothing went down [J-03]
- The verdict is a pure function of world, line and place, its medium and growth the incubator's (`mediumOfPlace`, `growthIn`) [J-03]
- Its order: furnace, liquid, medium, oxygen, pH, dose (SHIELD raises it), land (DRY), natives; the first failure is said [J-03]
- A native a tenth quicker there outgrows the line; a slower one it outgrows (fewer in samples); between, they live side by side [J-03]
- A line that takes drifts by a hash of world, line and place: one or two genes, never a vital one, twice as often under strong UV [J-03]
- A spread keeps to its line's waters (by salt), its land (DRY, or snow and ice) and its band of warmth, at its doubling's pace [J-03]
- A dye colours the waters a line lives in (the latest dye's), a glowing line lights them at night, from step 1; a layer of no look has no key [J-03]
- A vial keeps the guide's clock at its take: it holds a line only where the line had spread by then [J-03]

[J-01]: log/7-1.5.0.md
[J-02]: log/7-1.5.0.md
[J-03]: log/7-1.5.0.md
