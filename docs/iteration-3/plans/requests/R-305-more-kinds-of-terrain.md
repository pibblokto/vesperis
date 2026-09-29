---
id: R-305
title: More kinds of terrain than hills and mountains: canyons and a few other things
status: done 2026-09-28 (O10; O6-04/O6-05 of the terrain plan, in a different shape)
requested: 2026-09-28 (verbal)
related: R-304, PLAN-terrain-3 sections 6-7
---

## What
"Make terrain more interesting, add some other types of terrain besides hills, mountains and what else is there - come
up with something interesting. Canyons, I don't know, come up with a few things."

## My notes (Claude)
The landform library (`landformsAt` in `planetmap.cpp`, driven by the traits of R-304), each term fading with the
sampling scale so the map, the far rings and the ground agree, with a `mark` where the landform is at its most
distinctive (the galleries' finder) and a material where it shows (rock walls, basalt sheets, salt crusts, crystal):

| Trait | The landform |
| --- | --- |
| Canyon lands | a province of dry, flat, raised ground incised by a network of canyons (the boundaries of 20 km cells, 0.6-1.3 km wide, 150-600 m deep) and their side canyons (5 km, a third as wide and deep); the walls step down in three terraces |
| Badlands | flat-topped mesas 0.8-1.6 km across and 60-220 m high on half the cells of a 1.4 km grid, buttes and hoodoos on a sixth, steep rims with a talus foot, in a dry flat province |
| Karst towers | steep towers 50-210 m tall on half the cells of a 500 m grid in warm wet lowlands, sinkholes between |
| Great rift | one rift along the zero line of a broad noise: a flat floor 600-1500 m down between walls of three fault steps, raised shoulders, lakes on the floor of living worlds |
| Escarpments | one or two cliff lines thousands of kilometres long: a step of 150-600 m across the zero line of a broad noise, a talus foot then the cliff over 300 m, the height wandering along |
| Glaciated | ice fills the valleys of the cold high country, a crevassed tongue six metres under the smooth ground; fjords where the rivers of cold coasts reach the sea |
| Inselbergs | lone steep mountains 150-500 m tall and 0.8-3 km across on the plains, one per 25 km cell in three |
| Cinder fields | a volcanic province of cinder cones 140 m tall on a 2.5 km grid, fresh dark lava sheets on a third of the 5 km cells, strings of collapse pits |
| Chaos terrain | the crust broken into tilted blocks 2-6 km across, lifted or dropped by up to 80 m, cracks 90 m deep between |
| Patterned ground | a honeycomb of low ridges 40-120 m across on cold flat ground |
| Yardangs | wind-carved ridges along the wind, 80-200 m apart and 5-20 m high, broken into segments, in the desert flats |
| Dune seas | draa 1-2.5 km apart and 40-120 m high across the wind in the dry lowlands, sand |
| Salt flats | white playas in the closed basins of dry country, dead flat, cracked into polygons a dozen metres across (`MAT_SALT`) |
| Trap terraces | the ground quantised into steps of 40-90 m with steep risers, in a province |
| Great basin | one impact basin a fifth to two fifths of the radius across: a bowl 1.5-3 km deep flooded flat, a rim, a second ring, ejecta |
| Coronae | ring-shaped volcanic ridges 100-250 km across with a moat and a sunken centre (venusian, molten, volcanic) |
| Spire fields | crystal spires 10-35 m tall on quartz and carbon worlds; ice spires 3-8 m on icy worlds and comets (the 4 m ring's) |
| Geyser basins | sinter mounds 12 m high and 0.3-0.9 km across with 1.6 m terraces and a pool at the vent, pale crusts |

Also on every living world (R-306): the plains' own small relief (hummocks and shallow gullies), and the felisian
badlands terraces of M9-03 stay where the badlands trait is absent.

Open: the landforms are not yet landmarks (O6-06: named, marked on the map, logged), and the far rings carry only the
big ones (the terraces, the spires and the patterned ground live in the 16 and 4 m rings).
