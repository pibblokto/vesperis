# Plan: generation improvements (milestone M9)

## Where generation stands today

One planet function (`sampleSurface`) built from fractal noise, ridged noise, worley cells and jittered feature grids (craters, domes, volcanoes), gated by a detail scale so that the orbital map and the ground agree. It works and it is fast, but the worlds are still "noise-shaped":

* **Continents are blobs.** Land is a threshold on warped fbm; mountains are ridged noise masked to land. There are no chains, rifts, plateaus, island arcs, or coastlines shaped by geology.
* **No hydrology.** No rivers, lakes, deltas, glaciers; wet and dry regions come from a single moisture noise.
* **Few biomes.** Felisian materials are water, sand, grass, forest, rock, snow, ice; the same six everywhere, decided by three thresholds.
* **Craters are uniform.** Same profile at all scales, no ray systems, chains, basins, or flooding of old basins.
* **Deserts, poles and ice are latitude bands.** Ice caps are a latitude with noise; no seasons, glaciers, sea ice or dune seas.
* **Gas giants are static bands** with turbulence and a few storms; no differential rotation or evolving storms.
* **Systems are tidy.** Circular orbits, no belts, no resonances, moon types drawn from the same zone table as planets.
* **Statistics skew cold.** Around the start region felisian worlds are about 3% of bodies and roughly one per three systems; icy and cratered bodies dominate because most orbits are far out.
* **Maps are 256x128** and lit by albedo only, so relief is invisible from orbit.
* **No survey tools.** Tuning is done by eye on a handful of bodies.

## Goals

1. Worlds with structure you can read from orbit and recognise on the ground: mountain chains, rivers, coasts, dune seas, crater basins.
2. Variety between worlds of the same type and between regions of one world (biomes, climates, seasons).
3. Keep the single-function guarantee: everything is a pure function of the sphere point, gated by detail, consistent at every level.
4. Tune with data: a survey tool that measures distributions before and after a change.

## Items

| Id | Item | Size | Depends | Acceptance |
| --- | --- | --- | --- | --- |
| M9-01 | **Survey and gallery tools**: `vesperis_test survey` samples 2,000 bodies and prints type, temperature, gravity, moon-count and material-coverage distributions; `vesperis_test gallery <type>` renders 16 random landing sites of a type into one contact sheet; both used before and after every generation change | S | - | tools run in under a minute; output pasted into the request or PR |
| M9-02 | **Tectonic structure**: plates as worley cells on the sphere (5-12 per world); convergent boundaries raise mountain chains (ridged noise along the boundary band), divergent ones open rifts and mid-ocean ridges, island arcs along oceanic boundaries; continents shaped by plate interiors plus the existing warped noise | L | M9-01 | `map_FELISIAN` shows chains along boundaries; a landing at a chain shows long ridges instead of round hills |
| M9-03 | **Erosion look**: slope-dependent detail (rough on steep faces, smooth in valleys), valley widening at low altitude, talus and alluvial fans below cliffs, terraced plateaus and mesas in dry regions (quantised heights), badlands | M | M9-02 | gallery sheets show valleys and terraces; no "noise fuzz" on flats |
| M9-04 | **Rivers and lakes**: hashed river sources on high land, downhill traces baked into a coarse per-body flow map (about 2 km resolution) at first visit, fine function carves channels along the traced paths with width by accumulated flow, lakes in closed basins, deltas at coasts, wet materials along banks; frozen rivers where cold | XL | M9-02 | rivers visible on the map and walkable on the ground; a river reaches the sea in the test site |
| M9-05 | **Biomes**: temperature (latitude, altitude, ocean proximity) and moisture (noise, rain shadow from prevailing wind and mountains, distance to water) mapped to a biome table: tropical forest, savanna, desert, temperate forest, grassland, taiga, tundra, wetland, alpine, ice; each biome sets materials, vegetation density and family, colours; biome shown in the landing readout and data sheet | L | M9-04 | `map` colours follow biomes; the same world shows at least five biomes across latitudes |
| M9-06 | **Craters with history**: size-frequency power law per world age, ray systems from young craters (bright streaks visible from orbit), crater chains and secondary fields around large ones, multi-ring basins on cratered and icy worlds, mare flooding of old basins, ejecta blankets | M | - | `map_CRATERED` shows rays and a basin; ground view shows secondaries near a large crater |
| M9-07 | **Volcanism**: shield volcanoes with lava plains on thin-atmosphere worlds, volcanic islands and calderas on felisian worlds, cryovolcanoes and geysers on icy moons, hotspot chains; active ones glow on molten worlds only | M | M9-02 | at least one volcano type per world type visible in gallery sheets |
| M9-08 | **Dune seas**: barchan, transverse and star dune fields in deserts from a global wind field (direction by latitude band and seed), dune size by wind strength, bright sand albedo, dunes on felisian deserts too | M | M9-05 | dune field frame from `surface 5` and a felisian desert |
| M9-09 | **Ice and seasons**: polar cap radius from axial tilt and orbital position (the season), sea ice extent by season on felisian worlds, glaciers flowing down mountain valleys with moraines, ice shelves, snow line moving with season; the landing readout states the season | M | M9-05 | two landings six months apart at the same site show a different cap edge |
| M9-10 | **Gas giants alive**: differential rotation (zonal jets with their own periods), storms that drift and change size over days, polar vortex patterns, lightning on the night side, band structure visible from moons with M5-04 rings | M | - | globe frames at two times differ in storm positions |
| M9-11 | **New body types**: ocean world (global sea, ice floes, no land), iron/metal world (dark, glinting, cratered), tidally heated volcanic moon (Io-like, sulphur colours) next to gas giants, carbon world (black, graphite plains); each with map, surface look, palette, weights in the zone table | L | M9-01 | one gallery sheet per new type; survey shows sensible frequencies |
| M9-12 | **System architecture**: asteroid belts in orbit gaps, resonant moon periods (2:1, 3:2), captured retrograde moons far out, ring gaps carved by shepherd moons, eccentric orbits (with M5-03), warmer moons of gas giants from tidal heating | M | - | survey prints belt and resonance counts; a system list shows belts |
| M9-13 | **Galaxy regions**: star-forming regions (blue giants, nebula backdrop), old halo (white dwarfs and red giants), globular clusters (dense sectors), the core (pulsars and giants, high density), arms with dust lanes; regional name styles; region name shown in the space HUD | M | - | `stars` test prints counts by region; the map (M3-01) colours regions |
| M9-14 | **Descriptions**: a generated sentence for each body from its properties ("small icy moon with a fractured crust and a thin frost line") in the data sheet and log; star descriptions by class and region | S | - | text in `flow_data.png` |
| M9-15 | **Map quality**: 512x256 maps with slope shading (relief visible from orbit), normal-based limb lighting, generation moved to a worker thread so approach never stalls; the landing map zooms 2x around the cursor | M | M7-01 | globes show mountain shading; approach frame time under 3 ms while a map generates |
| M9-16 | **Distribution tuning**: rebalance zone weights and orbit growth so a typical system has one to two temperate worlds, fewer identical icy moons; more moon-type variety (moons draw from a moon table, not the planet table) | S | M9-01 | survey shows felisian at ~6-8% of bodies and at least one per two systems near the start |
| M9-17 | **Generation versioning**: `GEN_VERSION` recorded in saves and guide files; a change to any generator bumps it; on load the game warns that worlds may have changed but keeps the star coordinates valid; integer-only hashing kept for positions so star maps never move | S | - | loading an older save shows the warning once |

## Status (2026-09-26)

All seventeen items are implemented (details in `docs/reference/05-planet-function.md` and `04-galaxy-and-systems.md`). Deviations from the plan above:

* **Rivers (M9-04)** are not traced from a flow map. They follow a hashed network (worley cell boundaries at 80 km on land, 40-90% of the boundaries carry water depending on the body), carved as channels whose width grows toward low ground, with wet banks that raise the vegetation; lakes fill low basins; frozen where cold. Every water body carries its own surface level (`SurfaceSample::water`), so lakes and rivers render, reflect and are swum in above sea level. At the map's sampling scale a river is never thinner than the texel, so the network reads as lines from orbit and the real channel is found within the line when landing. A traced flow map remains an option if the network look proves too regular.
* Seasons (M9-09) change the polar caps, sea ice, snow line, tundra snow and frozen lakes; glaciers are only the ice above the snow line. Maps are regenerated per season step (eight per orbit).
* Gas giants (M9-10) get zonal jets (differential rotation of the static map), a polar vortex pattern and night-side lightning; storms do not evolve.
* Regions (M9-13) are not coloured on the star map yet; the space HUD names the region.
* No talus fans, crater chains, hotspot chains or eccentric orbits (M5-03).

Survey after the change (`vesperis_test survey 1500`): felisian 9% of bodies, in 54% of systems near the start; ocean 1.8%, metal 0.9%, volcanic 1.3%, carbon 2.9%; belts in most systems, 34 captured retrograde moons in 397 systems. The regression baseline was blessed for generation version 2.

## Rules that keep it consistent

* Every new feature is a function of the body-frame point and the body seed; no per-visit randomness.
* Coarse structures (plates, flow maps, wind fields) are computed once per body into small maps (kilometre resolution) that the fine function samples; those maps are themselves pure functions of the seed, so they can be recomputed anywhere.
* Feature amplitude fades with `lodFade` so the orbital map and the ground agree at low frequencies; materials depend only on values available at both detail levels.
* Any change is checked with the survey tool and the gallery sheets, and the reference note `05-planet-function.md` is updated in the same change.

## Order

M9-01 first (tools), then M9-02 and M9-03 (structure and look, the biggest visual gain per effort), M9-05 and M9-06 (biomes, craters), M9-04 (rivers, the hardest), then the rest by preference. M9-16 and M9-17 are small and can go early.
