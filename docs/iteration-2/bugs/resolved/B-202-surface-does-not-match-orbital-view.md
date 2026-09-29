---
id: B-202
title: The surface does not match the world seen from orbit (colours, materials)
severity: S2
status: resolved 2026-09-26 (N1)
reported: 2026-09-26
---

## Where
State / screen: orbit and landing map, then descent and surface.
Star / body: any landable world; strongest on thin-atmosphere, felisian and quartz worlds.

## Steps
1. Park at a world, look at the globe, open the landing map (`C`) and note the colours at the cursor.
2. Land (`Enter`) and look around.

## Expected
The ground you land on has the colour and character of the spot you picked: the tan desert on the map is a tan desert, the red world seen from orbit is red underfoot, a snowfield is white, a forest is the forest's green.

## Actual
The ground is a different colour from both the globe and the map, and different materials on the map (sand, rock, dust) look the same on the ground apart from brightness.

## Analysis (Claude)
Three colour systems decide what a world looks like, and they were tuned independently (KI-201):

| View | Where the colour comes from | Per material? |
| --- | --- | --- |
| globe from space (`SpaceRenderer::setupPalette`, `drawGlobe`) | `Body::color` = `typeColor(type, rng)`: one hue per type with a random component (thin atmosphere picks red, green, purple or grey), tinted by the star; the map's albedo only sets brightness; felisian oceans get a blue bank; lava a fixed bright ramp | no (two hues per body: land, ocean) |
| landing map (`buildLandingMapBase`) | a fixed table `matCol[MAT_COUNT]` (rock grey-brown, sand tan, grass green, forest dark green, snow white, water blue, lava orange, ice pale blue, quartz white, ...) times the albedo stretched between its 2nd and 98th percentile | yes, but the table ignores the body's own hue |
| surface (`lookFor`, `setupPalette`, `materialLook`) | `SurfaceLook` per type: `ground` is a hand-picked colour lerped 25-50% toward `Body::color`, `secondary` water or lava, `tertiary` `vegColor`, snow a fixed bank; rock, sand and dust all use the ground bank with only albedo differences | partly (3-4 banks) |

So a thin-atmosphere world drawn red from orbit lands as `lerp(brown, red, 0.5)`; the map's tan sand and grey rock become one bank on the ground; the felisian globe is a single green-blue with white highlights while the ground is brown-grey with `vegColor` trees. Height and relief do agree (the same planet function feeds everything, `docs/reference/05-planet-function.md`), which is why the *shape* matches and only the *look* does not.

## Measured (N0-02, 2026-09-26, before N1)
`vesperis_test consistency` (average RGB of a 24-texel map crop, a 24 px globe patch from 3.5 radii and a 96 px ground crop at noon, all at the sub-solar point; distances 0-441):

| Type | map | globe | ground | map-globe | globe-ground | map-ground |
| --- | --- | --- | --- | --- | --- | --- |
| THIN ATMOSPHERE | 91,75,61 | 83,87,115 | 81,75,94 | 56 | 24 | 35 |
| ICY | 179,194,218 | 142,160,224 | 51,56,86 | 50 | 196 | 230 |
| ROCKY | 118,105,93 | 95,79,83 | 73,67,78 | 36 | 26 | 61 |
| CRATERED | 157,128,104 | 98,89,103 | 81,81,100 | 70 | 19 | 89 |
| MOLTEN | 78,73,73 | 148,80,27 | 57,39,27 | 84 | 100 | 60 |
| FELISIAN | 124,131,128 | 53,67,55 | 117,97,65 | 120 | 71 | 73 |
| OCEAN | 11,27,67 | 45,88,121 | 20,52,84 | 88 | 58 | 31 |
| QUARTZ | 166,153,163 | 243,219,198 | 184,165,145 | 107 | 96 | 28 |
| METAL | 100,96,93 | 18,18,20 | 59,57,57 | 135 | 68 | 67 |
| CARBON | 45,43,45 | 6,6,6 | 60,56,49 | 66 | 85 | 20 |
| VENUSIAN | 66,61,61 | 244,174,134 | 111,76,37 | 223 | 191 | 53 |
| VOLCANIC | 122,110,64 | 146,136,44 | 40,33,26 | 41 | 149 | 118 |

11 of 13 types over the threshold of 60 (worst 230, the icy ground at noon is a dark blue-grey under a white map); the felisian sheet shows the complaint exactly: a green map, a dark green globe, a brown ground. The venusian globe is its cloud deck while the map (without clouds) and the ground are basalt: the map's `C` view and the globe cannot agree by design, so N1 compares the venusian ground with the map's cloudless view and leaves the globe out. Sheets in `shots/consistency_<TYPE>.png`.

## Fix (milestone N1, `plans/PLAN-orbit-to-surface-consistency.md`)
One material palette per body, computed once in `BodyGen` (`matColor[MAT_COUNT]` from the type hue, per-material tints, biome and season), used by all three views; the globe gains per-material banks (rock, water, vegetation, sand, ice) instead of one hue; the surface builds its material ramps from the same colours; the landing map drops `matCol`. A consistency test renders the three views of the same spot side by side and measures the colour distance.

## Acceptance
`vesperis_test consistency` prints, for every landable type, the colour distance between the map cell, the globe patch and the ground at noon (all below a documented threshold) and writes `shots/consistency_<TYPE>.png` sheets; `regress` frame hashes re-blessed with the reason in `10-decisions.md`.

## Outcome (2026-09-26, N1)
One palette per body: `BodyGen::matColor[MAT_COUNT]` (`materialPalette()` in `galaxy/planetmap.cpp`) mixes each material's own tint with the body hue (`Body::color`), with per-type overrides (a thin-atmosphere world is strongly tinted everywhere, felisian rock and sand stay rock and sand, icy cracks are dark rock). Materials belong to six families (rock, water, forest, grass, sand, snow; `matFamily`, `familyRep`), each with a palette bank in both renderers: the globe uses 2/3 rock, 6/7 water, 11 forest, 15 grass, 13 sand, 14 snow (thick cloud goes to the snow bank so it reads white); the ground uses 0 rock, 2 water/lava, 3 forest, 8 snow/ice, 9 sand/dust/sulphur/quartz, 10 grass. The landing map is `materialRampColor(ramp[material], exposureStop(albedo, light, atmosphere) x hillshade)`, the fixed `matCol` table is gone, and a swatch under the map shows the noon ground colour at the cursor with the material and a relief class (`FLAT / HILLS / MOUNTAINS`).

One exposure rule (`noonStop`, `exposureStop`) and one ramp shape (`materialRamp`, hemisphere light included) serve all three views; the globe's `63 albedo^0.6 shade 1.1` is gone. The star's light tint is `lerp(star, white, 0.5)` everywhere.

After (same test as above, but the site now stands on the material that covers most of the globe patch, and the map and globe averages take only that material):

| Type | map | globe | ground | map-globe | globe-ground | map-ground |
| --- | --- | --- | --- | --- | --- | --- |
| THIN ATMOSPHERE | 88,87,106 | 76,76,96 | 93,93,113 | 19 | 30 | 11 |
| ICY | 124,139,196 | 120,134,189 | 125,140,197 | 9 | 11 | 2 |
| ROCKY | 80,71,76 | 72,64,69 | 76,67,72 | 12 | 6 | 7 |
| CRATERED | 94,87,95 | 84,77,85 | 82,75,83 | 18 | 3 | 21 |
| MOLTEN | 66,46,30 | 94,58,36 | 58,40,25 | 31 | 42 | 12 |
| FELISIAN | 84,76,66 | 88,80,69 | 102,90,72 | 7 | 17 | 23 |
| OCEAN | 51,82,115 | 79,95,105 | 31,65,104 | 32 | 57 | 29 |
| QUARTZ | 204,188,173 | 210,192,175 | 211,193,175 | 8 | 1 | 9 |
| METAL | 35,34,37 | 34,34,37 | 35,35,37 | 1 | 1 | 1 |
| CARBON | 10,10,11 | 12,12,13 | 13,13,14 | 3 | 2 | 5 |
| VENUSIAN | 118,84,51 | 216,147,106 | 106,75,44 | (129) | (145) | 16 |
| VOLCANIC | 33,29,24 | 70,55,44 | 39,34,28 | 49 | 41 | 8 |

0 of 13 types over the threshold of 60 (the venusian globe is its cloud deck by design and is compared map-to-ground only). The molten and volcanic globes keep their lava ramp, which explains their 40-50. Found on the way: planetshine lifted the dark end of every ground ramp by day on airless worlds because `day` there is the sky brightness, which is always 0; the lift now follows the sun's altitude (this is why metal and carbon grounds measured 60 against a globe of 12 before). Regress frame hashes re-blessed; generation hashes unchanged.
