---
id: R-202
title: Animals are bad
status: done 2026-09-26 (N3; no binocular zoom and no naming of species by the explorer, see KI-209)
requested: 2026-09-26
related: N3-01..N3-06, KI-204, KI-010, M4-05 (first attempt)
---

## What
Real creatures: animals with bodies, legs that walk, heads that graze, herds that react, birds that land in trees, swimmers, night animals; species that differ between worlds and can be named.

## Why
Boxes on line legs break the illusion the moment you look at them. Creatures are the second half of a living world and the thing you tell someone about.

## Must / should / could
- must: articulated animated bodies; several species per world with different plans and sizes; herd, graze, drink, rest, alert and flee behaviours; birds that land and take off; a call per species.
- should: curiosity, stampedes from the buggy, tracks, the guide naming species and logging first sightings, encounter density tuned per biome.
- could: predators, migrations, insects at dusk, a creature that follows you.

## References
`shots/scene_felisian_herd.png` for the current boxes.

## My notes (filled by Claude)
Design in `PLAN-living-worlds-2.md`; items N3-01 to N3-06 in `ROADMAP.md`. Size XL. A bestiary generator (deterministic per world) feeds articulated segment bodies with gait cycles and a behaviour state machine; sounds from the synth. Acceptance: `vesperis_test bestiary`, herd scene sheets, a flow step approaching a herd that logs alert and flee, encounter density over a survey, budget under 0.5 ms for a herd of ten at 20 m.

### Outcome (2026-09-26, N3)
`surface/bestiary.*` gives every living world a deterministic bestiary (`Bestiary::make`: 2-4 land species, 1-2 flyers, 1 swimmer, 0-1 nocturnal; body plan, size, gait, hide bank and tone, pattern, crest, habitat, diet, activity, temperament, call, herd size, a generated name). `surface/creatures.cpp` spawns herds from it at a site (`planLife`, shared with the encounter survey), runs the behaviour state machine (idle, walk, graze, drink, rest, alert, flee, curious; leaders and followers; rest at midday and at night or reversed for night and dusk species; walks to water in the afternoon; alert then flight from the explorer within 25 m and from the moving buggy within 60 m with a stampede through the herd; curious species approach to 10 m; giants ignore everything) and draws articulated bodies: three trunk segments, a neck and head pitched by the plan and lowered to graze or drink, a two-segment tail, jointed legs with gait cycles per plan (walk, trot, gallop, hop, tripod crawl, stride, folded when lying) whose feet stand on the terrain, eyes, horns, ears, crests or antlers, stripes, spots or countershading, tracks in sand and snow. Flyers flap and glide, land, and perch in the tallest tree near their circle at dusk (dusk species reversed); swimmers show a fin, a back arc and the odd breach. Calls (chirp, low, hoot, click), hoof-steps within 20 m and a dusk insect hum go to the synth. First sightings within 60 m go to the expedition log (`SIGHTING`) and a `CREATURES SIGHTED` statistic; the data sheet lists the world's species (sighted ones starred); the landing map names the species most likely met in the cursor's biome; `X` shows species names and states.

Numbers: `vesperis_test bestiary` prints six worlds and checks determinism; `encounters` over 300 random felisian land sites: a herd within 200 m at 86% of tropical, 100% of temperate, 80% of grassland, 75% of savanna, 56% of taiga, 30% of tundra, 50% of desert, 15% of alpine and 0% of ice sites (most random sites on the start region's cold living worlds are ice, so the overall figure is 11%); the flow walks into a herd: alert, calls and four fleeing; a herd of nine at 18 m costs 0.05 ms to draw at 1x (budget 1 ms). Scenes `felisian_herd_near/_/far` (6, 18, 60 m on open grassland) and `creatures_lineup` (one of each land species side on). Not done: the binocular zoom and naming species in the guide (KI-209).
