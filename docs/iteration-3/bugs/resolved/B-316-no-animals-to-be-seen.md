---
id: B-316
title: No animals to be seen, only the odd flock of birds
severity: S3
status: resolved 2026-09-28
reported: 2026-09-28
---

## Where
State / screen: surface, felisian worlds
Star / body shown in the HUD: any
EPOC shown in the HUD: n/a
Save attached: no

## Steps
1. Land on a living world, walk or drive around for a while.

## Expected
Herds wherever the biome has them.

## Actual
"I don't see any animals for some reason, sometimes birds and that's it" (the user).

## Notes
Herds were planned once per landing within 180 m of the capsule (with the biome's chance, 85% on grassland) and
that was all the life of the site: walk or drive away and there was nothing, and the re-anchor at 50 km cleared even
those. B-316 (`surface/creatures.cpp`): habitat cells, 512 m squares of the body's latitude and longitude (like the
ruins' 2 km cells, so they stay put through a re-anchor), each with a hashed chance of a herd by its biome (grassland
and savanna 0.55, temperate, tropical and wetland 0.4, taiga 0.3, desert and tundra 0.18, alpine 0.1), a species from
the bestiary, a size and a spot on dry land (`planCellHerd`); `updateHabitat` (every half second) spawns the herds of
the cells whose spot comes within 900 m of the explorer and forgets those beyond 1300 m; the landing's own herds stay as
before. `SurfaceView::herds` lists them all (`Critter::herd` indexes it), a herd keeps within 300 m of its spot, and
the big species are drawn to 600 m (350 before). `unit`: the grassland herd site has 2 landing herds and 7 habitat
herds (49 animals) within reach; 3 km on, the herds behind are forgotten and 7 new ones are in reach. `encounters`
prints the habitat herds within 900 m per site.
