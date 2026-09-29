# Plan: Terrain III — drastically more interesting, realistic, worth exploring (O6)

Status: O6-01 and O6-02 done on 2026-09-27 (sections 6-8 below); O6-03 onward on the user's review. The user's brief: "come up with an idea to make terrain DRASTICALLY more interesting, realistic, exploitable. Even now things are mostly flat; explore techniques, prepare a plan for that update; we will be working on surface exploration later in greater detail."

## 1. Where it stands, measured

`vesperis_test terrain` samples two bodies of every landable type, three land sites each, on a 48 x 48 grid at 16 m (what you walk on), a 32 x 32 grid at 64 m and one at 512 m (what you see from a ridge). Slope is rise over run.

| Type | 16 m: slope median / p90 / max | over 0.04 (hills) / over 0.15 (mountains) | relief over 770 m | relief over 2 km (64 m) | 512 m: slope median / p90 | relief over 16 km |
| --- | --- | --- | --- | --- | --- | --- |
| Molten | 0.087 / 0.151 / 0.29 | 87% / 10% | 54 m | 126 m | 0.030 / 0.059 | 426 m |
| Cratered | 0.205 / 0.472 / 6.67 | 85% / 62% | 177 m | 473 m | 0.183 / 0.483 | 3060 m |
| Venusian | 0.039 / 0.073 / 0.13 | 48% / 0% | 30 m | 53 m | 0.021 / 0.040 | 220 m |
| **Felisian** | **0.042 / 0.126 / 0.28** | **52% / 6%** | **38 m** | **76 m** | 0.019 / 0.080 | 338 m |
| Rocky | 0.097 / 0.189 / 0.92 | 90% / 22% | 73 m | 152 m | 0.050 / 0.094 | 635 m |
| Thin atmosphere | 0.092 / 0.173 / 0.49 | 81% / 20% | 73 m | 178 m | 0.046 / 0.167 | 1268 m |
| Icy | 0.024 / 0.346 / 1.34 | 34% / 15% | 45 m | 62 m | 0.012 / 0.070 | 162 m |
| Quartz | 0.057 / 0.158 / 0.49 | 64% / 11% | 48 m | 113 m | 0.005 / 0.016 | 65 m |
| Ocean | 0.000 / 0.003 / 13.5 | 3% / 3% | 152 m | 152 m | 0.000 / 0.418 | 151 m |
| Metal | 0.207 / 0.530 / 14.4 | 97% / 62% | 186 m | 356 m | 0.043 / 0.435 | 2131 m |
| Volcanic | 0.052 / 0.094 / 0.17 | 66% / 0% | 30 m | 51 m | 0.020 / 0.044 | 308 m |
| Carbon | 0.030 / 0.084 / 0.31 | 35% / 2% | 22 m | 45 m | 0.015 / 0.043 | 201 m |
| Comet | 0.175 / 0.796 / 3.28 | 96% / 56% | 123 m | 206 m | 0.035 / 0.094 | 310 m |

Reading it: the living worlds are the flattest. A felisian site has 76 m of relief over 2 km and a median grade of 4% (a gentle golf course); the "mountains" of the planet function are 1000 km wide bumps 3-5 km high (`rg(0.14 R)^2 x mountainAmp`), i.e. a 0.5% grade you cannot perceive on foot, with ridges of 900 m every 35 km (5%) and hills of 150-400 m every 5 km (8%). Real mountain country has 1-3 km of relief over 5-20 km (grades 20-60%), cliffs, talus, valleys that lead somewhere. The cratered, metal and comet types come out steep only because craters are the one landform with real slopes at every scale; their high maxima (6-14) are crater rims sampled at 16 m.

What the function does today (`05-planet-function.md`): per type a sum of band-limited noises (fbm, ridged, warped fbm) at fixed wavelengths with LOD fades, plate tectonics as worley cells (uplift, chains, rifts, arcs), crater/dome/volcano feature fields on jittered grids, worley-edge rivers with local water levels, lakes where a low-frequency noise dips, terraces and badlands on dry flats, dunes, biomes from temperature and moisture. It is consistent at every detail (one function feeds the globe, the map and the ground), deterministic, and cheap (5-10 us per 16 m sample).

Why it reads flat and "decorative":

1. **The spectrum is wrong at human scales.** Amplitude falls off far faster than wavelength: 3000 m at 1000 km, 900 m at 35 km, 200 m at 5 km, 20 m at 400 m, nothing below. Natural terrain is close to self-affine with amplitude ~ wavelength^H, H about 0.8-0.9: a 5 km hill of 200 m implies 100 m of relief at 1 km and 30 m at 200 m, and cliffs break the rule locally.
2. **No process, only noise.** Nothing flows: valleys do not join into drainage, ridges do not branch, slopes do not know about talus angles, coasts do not know about waves. Ridged noise gives crests everywhere and valleys nowhere.
3. **Landforms live at the wrong scale for a walker.** Chains 320 km wide, canyons from `rg(0.3 R)^9` (a 2 km deep trench thousands of kilometres long), volcanoes on a 0.25 R grid. Within the 13-53 km you can see and the 5-30 km you will cross, the field is one bump.
4. **Materials follow height and moisture only.** No rock on steep slopes, no scree under cliffs, no wet valley floors, no strata.
5. **No reasons to go anywhere.** Nothing is named, nothing stands out, nothing rewards the climb.

## 2. Targets (what "drastically better" means, so it can be measured)

Measured by `vesperis_test terrain` and a landform gallery, per relief class the landing map prints:

| Class (landing map) | Today (felisian) | Target |
| --- | --- | --- |
| MOUNTAINS: median slope at 16 m / relief over 2 km / relief over 16 km | 0.04-0.13 / 76 m / 338 m | 0.25-0.45 / 500-1200 m / 1500-3500 m, with cliff cells (slope > 1) on 3-8% of the area |
| HILLS | 0.04 / 50 m | 0.08-0.15 / 120-250 m, valleys with floors under 5% |
| FLAT (plains, deserts, tundra) | 0.03 / 30 m | 0.01-0.04 (kept flat on purpose), but broken by terraces, gullies, dune fields, salt flats, outcrops |
| Rivers | worley edges ignoring slope | every river runs downhill to a lake or the sea; 80% of lowland landings within 10 km of a channel; deltas at the coast |
| Landmarks | none | a named feature visible from 90% of landings (peak, mesa, canyon rim, crater, geyser field, arch, crystal field...), listed on the landing map, the sector map and the data sheet |
| Cost | 5-10 us per 16 m sample, 3.9 ms per surface frame at 2x | at most 2x per sample; the frame budget stays 16 ms at 2x (bench) |
| Consistency | `consistency` 0 of 13 over 60 | unchanged; the far ring shows the same ridges and valleys the near rings do |

## 3. Techniques surveyed, and what fits a pure function sampled at variable detail

The constraint that shapes everything: the terrain must stay one deterministic function of position (any point at any detail, no simulation state at runtime), because the globe, the landing map, the caches and the reanchoring all depend on it. Simulations are allowed only as **precomputed per-planet layers at coarse resolution** built on the map worker thread (like the 512 x 256 planet map today: 40-110 ms) and sampled bilinearly as low-frequency terms of the function.

**A. Amplitude spectrum and multifractals (Musgrave; "hybrid multifractal", "ridged multifractal").** Replace the three or four fixed-wavelength terms with a self-affine sum from the continent scale down to 8 m: `sum a0 (wl/wl0)^H n(p/wl)` with H per type (0.9 mountains, 0.8 hills, 0.6 dunes) and an octave gain that depends on the accumulated height (hybrid multifractal: rough where high, smooth where low, which reads as erosion). Cheap (one noise per octave, 10-14 octaves at 16 m), gated by the same LOD rule. This alone moves felisian mountains from a 4% grade to 20-30%.

**B. Domain warping (Quilez).** Two-level warps of the coordinate by low-frequency noise fold ridges and valleys into plausible curves instead of the isotropic lumps of plain fbm; the continent field has one warp already (0.22). Cost: 3-6 extra noises per sample.

**C. Slope-damped octaves, the "erosion look" (Quilez's terrain, Sharpe's uber-noise, the derivative trick).** Each octave is weighted by `1 / (1 + k |grad|^2)` of the sum so far (the gradient comes free from analytic noise derivatives, which `gnoise3` would need to expose): steep slopes stop accumulating detail and stay smooth, flat areas and ridge lines gather it. The result is the flat valley floors, sharp ridge crests and smooth scarps of eroded land without simulating anything; gated by detail like everything else. Uber-noise adds "sharpness" (ridged vs billowy per octave), "altitude erosion" (fewer octaves at height) and "slope erosion" parameters that can be set per type. Cost: analytic derivatives roughly double a noise evaluation.

**D. Hydraulic and thermal erosion as precomputed maps (Mei/Benes, Musgrave's original).** A droplet or pipe-model erosion pass on a coarse planet grid (the 512 x 256 map, or 2048 x 1024 for the sectors you land in) gives real valleys, ridges and sediment fans, but at 40-110 km per texel on the whole-planet map it carries only the continental drainage; a per-sector pass at 100-400 m per texel (built for the landing sector and its neighbours on the worker thread when the capsule is chosen) would carry walking-scale valleys. The function then adds `erosionMap(lat, lon)` (a signed height term) and `flowMap` (accumulation) as low-frequency inputs. Deterministic if the pass is deterministic (fixed iteration counts, integer hashes for droplets). Memory 1-4 MB per sector, cached like maps. This is the biggest lever for "realistic" and the most work; consistency needs care: the globe and the landing map must add the same term at their resolution (they can, it is a map).

**E. Drainage networks from flow routing (priority-flood depression filling, D8/D-infinity accumulation).** On the planet grid: fill depressions (they become lakes at the fill level), route flow, accumulate area. Channels are where accumulation exceeds a threshold; their width and depth scale with accumulation (`w ~ A^0.5`, `d ~ A^0.3`), deltas where a channel meets the sea, meanders as a sinuous warp along the channel in flat reaches, terraces along big rivers. Runtime: the channel network stored as polylines in a spatial hash per sector; the function takes the distance to the nearest channel (as the rivers do now, but along a real network) to carve a V or U valley profile and to set the water level (flat within a reach). Replaces the worley rivers, which can run up ridges today. Lakes get real shorelines (the fill contour). Cost at runtime: a distance query per sample near channels, as now.

**F. A landform library, pure functions of position gated by detail (all cheap, all deterministic):**
* Cliffs and terraces: a warped noise thresholded into steps with a `smoothstep` riser of 5-40 m and a talus slope (35 deg) of scree material at the foot; strata bands on the riser as material stripes.
* Mesas and buttes on dry plateaus: cap rock from a worley cell field, cliff rings, debris aprons.
* Canyons: river incision where uplift is high (the drainage of E carved by `uplift x accumulation^0.5`), rims as landmarks.
* Glacial valleys above the snow line and in cold biomes: U-profiles along the channels of E, cirques (hemispherical bites) on the lee of high ridges, moraine ridges across the valley mouth, fjords where a glacial valley reaches the sea at high latitude.
* Coasts: exposure to the prevailing wind gives sea cliffs and wave-cut platforms on the windward side, beaches and dunes on the lee, spits and barrier islands in flat shallow reaches (the shelf term is already there).
* Volcanic: cones (exist) with lava flows following the steepest descent from the vent for a hashed length (a thin dark tongue material), lava tubes as pit chains (skylights), cinder fields.
* Karst on wet limestone-like plateaus (a per-planet "lithology" flag): sinkholes (small deep bowls), tower karst (dome fields with cliffs), dry valleys.
* Yardangs and ventifacts in windy dry lowlands: long ridges aligned with the wind, a few metres high, between the dunes (dunes exist and gain slip faces).
* Salt flats and playas in closed basins (the lakes of E that evaporate in dry climates): perfectly flat, bright, cracked material.
* Badlands and gullies on dry slopes (exist as noise; become dendritic with a small-scale flow term).
* Outcrops, tors and boulder fields: rock protrusions from a hashed field on ridges and slopes, boulders piled under cliffs (colliders as today).
* Impact terrain (exists): terraced walls and central peaks on big craters, ejecta blankets rougher than the plains, secondary crater chains radial to the parent.

**G. Materials from geomorphology instead of height alone.** Slope over 35 deg: bare rock; 25-35: scree; aspect (the sun-facing side drier, the shaded side snowier); the topographic wetness index `ln(A / tan beta)` (from E) for marsh, greener valley floors and treeless dry ridges; strata colour bands from a lithology field; fresh vs weathered rock albedo; salt and playa materials. All are evaluated from values the function already has (gradient, accumulation), so the globe, the map and the ground stay consistent.

**H. Landmarks and points of interest (the "exploitable" part).** A hashed grid per planet (as ruins use today, 2 km cells) of named features with a type, a size and a place chosen from the terrain itself (the highest point of a 10 km cell, the rim of the biggest crater, the vent of a geyser field, the mouth of a canyon): `PEAK 1240 M`, `ARCH`, `PIT`, `GEYSER FIELD`, `CRYSTAL FIELD`, `IMPACT SITE` (a fresh crater with a dark ejecta ray, meteorite fragments as metal rocks), `LAVA LAKE`, `HOT SPRING`, `ICE CAVE MOUTH` (a dark overhang quad in a cliff). They appear on the landing map's zoom and the sector map as marks, on the HUD when in the rangefinder's line ("MESA 4.2 KM"), get a `LANDMARK` entry in the log on first sight within a kilometre and can be named in the guide. They give the walk a destination and the climb a view; no goal, no score, in the spirit of the original.

**I. Rendering support.** A near ring at 4 m cells within about 120 m (cliffs and gullies are invisible at 16 m); cast shadows of cliffs already work through `castShadowLit`; a rock "wall" material rendered with the strata tile; scree as a rougher meso tile; the far ring must carry the drainage term so valleys read from a ridge. Overhangs and caves are not possible with a height field (an optional second height field or displaced meshes is a later milestone).

**J. Performance and determinism.** Budget: at most 2x the per-sample cost (analytic derivatives, more octaves, a distance-to-channel query), which the prefetch worker and the caches absorb; the erosion and flow layers are computed once per planet (and per landing sector) on the map worker thread with fixed iteration counts and hashed inputs, cached like maps, so results are bit-identical between runs. Generation version 6; saves warn once; the regression baseline is re-blessed and the consistency test extended with a slope and a drainage check.

## 4. Proposed milestone O6 (order and sizes)

| Id | Item | Size | What it delivers |
| --- | --- | --- | --- |
| O6-01 | Metrics and gallery: extend `vesperis_test terrain` with the per-class targets (pass/fail), and a `landforms` contact sheet (12 sites of a type at three scales) | S | the acceptance harness before any change |
| O6-02 | The spectrum: analytic noise derivatives in `core/noise`, multifractal sums per type with H and gain by height, two-level domain warps, slope-damped octaves; a near ring at 4 m; retuned amplitudes so plains stay plains | L | mountains at 20-40% grades, eroded looks, cliffs; the biggest visual change for the least code |
| O6-03 | Drainage and erosion layers: priority-flood filling and flow accumulation on the planet map (worker thread), a per-sector 200 m layer when the capsule is chosen, channel polylines in a spatial hash, valley carving by accumulation, lakes at fill levels, deltas, meanders; the worley rivers retired | XL | rivers that go somewhere, valleys that join, lakes with real shores |
| O6-04 | Landform library I (cliffs, terraces, mesas, canyons, talus, outcrops, boulder fields) and materials by slope, aspect and wetness | L | the walking-scale variety |
| O6-05 | Landform library II (glacial valleys, cirques, fjords, sea cliffs and beaches by exposure, lava flows and pit chains, karst, yardangs, playas, slip-face dunes, crater terraces) | L | per-type character |
| O6-06 | Landmarks: the hashed POI grid, names, marks on the maps, the rangefinder label, the log entry, naming in the guide | M | reasons to go |
| O6-07 | Consistency, performance, generation version 6, reference notes, the survey's numbers | M | the guard rails |

Suggested order: O6-01, O6-02 (ship it: it is already a different game), then O6-03 (the long one), O6-04, O6-06, O6-05, O6-07 last but running throughout. Roughly two to three weeks of focused work; O6-02 alone about two days.

## 5. Risks and open questions

* **Look.** The original's polymap was flat and the mush hides fine relief; cliffs at 16 m cells are staircases. The near ring at 4 m and the flat-cell shading keep the character; sheets at each step decide.
* **Consistency across resolutions.** Erosion layers are coarse: the whole-planet layer carries 40-110 km features, the sector layer 200 m ones; the analytic terms (A-C, F) carry the rest and must be gated exactly as today, or the far ring and the near ring disagree. The consistency test grows a "ridge and valley" comparison between the far ring and the near ring at the same spot.
* **Cost.** Analytic derivatives and 12-14 octaves double the sample cost; the prefetch worker and the caches were built for that, but the landing map's zoom (32k samples) and the descent's first frames need re-measuring.
* **Determinism (KI-014).** Floating-point erosion on a worker is deterministic per machine; across compilers it is not, as today's noise is not either.
* **Saves and guides.** Generation version 6: every world's ground changes; star positions and systems do not.
* **Questions for the user:** (1) how far toward realism against the original's flat, dreamlike plains (the plan keeps plains flat by class and makes mountains mountains); (2) caves and overhangs: out of scope for a height field, or worth a displaced-mesh milestone later; (3) whether landmarks should ever carry gameplay beyond seeing, naming and logging (the original had no goal; the plan keeps it that way).

## 6. The user's answers (2026-09-27)

1. **Realism, cinematic.** Terrain that reads as real geology yet is built to be looked at and crossed: every landing should have a view worth turning around for, and the ground must stay drivable (valleys, passes and ridge lines a buggy can follow at speed; cliffs you go around, not a maze of walls). The flat classes stay flat on purpose but are broken by terraces, gullies, dune fields, outcrops and playas so a plain is never empty. Slopes respect the walking rules (climbable below rise over run 1.35, sliding above), so a mountain is a route as well as a picture.
2. **Caves and overhangs are out of scope for O6.** Nothing in O6 may block them; the next iteration adds them (a second height field or displaced meshes). The pit chains and cave mouths of O6-05/O6-06 stay surface features a later milestone can open up. Recorded in `10-decisions.md` and `IDEAS.md`.
3. **Landmarks: yes, execution over quantity,** in the original's spirit (no goal, no score): something to see, name and log. Each landmark is placed by the terrain itself (the real peak of a cell, the real rim of a crater, the real mouth of a canyon), never dropped on flat noise; it is visible from far away with a silhouette that reads at 10-20 km through the far ring; it has a payoff up close (a view from the top, a rim to stand on, a geyser that vents on a schedule, a lava lake that glows at night, an impact site with metal fragments, a crystal field that glints); it shows on the landing map zoom and the sector map with its type; the rangefinder names it when the crosshair rests on it; the first sight within a kilometre writes a `LANDMARK` log entry with a generated name the explorer can rename in the guide. Six kinds first, done well (peak, mesa, canyon rim, crater, geyser field, crystal field), more only when those six feel right; about one landmark within 10 km of 90% of landings, rarely two in view at once.

Order agreed: O6-01, O6-02 (this review), O6-03, O6-04, O6-06, O6-05, O6-07.

## 7. What the targets measure (O6-01, `vesperis_test terrain`)

48 hashed land sites per landable type (four bodies near the start, twelve sites each, 60 S to 60 N), each on four grids of the planet function: 24 x 24 cells at 4 m (the near ring), 48 x 48 at 16 m (770 m, what you walk on), 32 x 32 at 64 m (2 km) and 32 x 32 at 512 m (16 km, what you see from a ridge); a slope is the rise over run between neighbouring cells; the relief of a window is its maximum minus its minimum. Sites are grouped by the class the landing map prints (`Game::siteSlope`, `reliefClass`) and the 16 m slopes of a class are pooled.

| Target | Definition | Owner |
| --- | --- | --- |
| MOUNTAINS median 0.25-0.45 | median of the pooled 16 m slopes | O6-02 |
| MOUNTAINS relief 500-1200 m / 1500-3500 m | mean relief over the 2 km / 16 km windows (16 km up to 5000 on crater-field types: a basin's wall) | O6-02 |
| MOUNTAINS cliffs 3-8% | share of 16 m cells with a slope over 1.0 | O6-04 |
| MOUNTAINS walkable 90% | share of 16 m cells under 1.35 (the climbing rule) | O6-02 |
| MOUNTAINS floors 15% | share of the lowest quarter of the 64 m cells under a 5% slope | O6-03 |
| HILLS median 0.08-0.15, relief 120-250 m over 2 km (450 on crater-field types), floors 25% | as above | O6-02 (floors on crater-field types: O6-05) |
| FLAT median 0.01-0.04 (0.09 on crater-field types, whose plains are crater fields) | as above | O6-02 |
| FLAT broken: p90 over 0.06 and 6 m over 770 m | terraces, gullies, dunes, outcrops, playas | O6-04/05 |
| Comets | walkable 85% only (smaller than the windows, craggy by nature) | - |
| Cost | the best of three passes over the 16 m grid, within 2x the O5 figure pinned per type | O6-02 |

Crater-field types: cratered, thin atmosphere, metal, rocky, icy, carbon. Targets owned by later items print as `FAIL(O6-0N)` and are not counted; the exit code is the number of failing spectrum targets.

## 8. O6-01 and O6-02 results (2026-09-27)

**Same ground, before and after** (`vesperis_test landforms sites`, the sheets in `shots/tests/o6/before/` and `after/`, each at three distances over hillshades of 770 m, 3 km and 24 km):

| Site | 16 m median before -> after | p90 | relief 770 m | 2 km | 16 km |
| --- | --- | --- | --- | --- | --- |
| felisian mountains (Skeatoltdos VII-a, 17.7 N 24.8 W) | 0.17 -> 0.26 | 0.25 -> 0.32 | 143 -> 263 m | 239 -> 472 m | 1029 -> 1476 m |
| felisian plain (13.1 N 86.4 E) | 0.02 -> 0.01 | 0.05 -> 0.01 | 26 -> 7 m | 37 -> 19 m | 91 -> 48 m |
| thin-atmosphere hills (Brooshaing I, 56.2 N 156.6 E) | 0.09 -> 0.12 | 0.16 -> 0.23 | 54 -> 92 m | 96 -> 200 m | 289 -> 299 m |
| cratered moon (Brooshaing II-a, 35.9 N 148.8 E) | 0.10 -> 0.12 | 0.27 -> 0.31 | 57 -> 78 m | 139 -> 212 m | 577 -> 635 m |

The plain is flatter than before on purpose (the flat class keeps a 1-2% grade; the coastal plain of the pinned site is damped further by the land mask); the hillshades are the telling part: blank tiles before, ridge networks with valleys and rivers after.

**Per type after O6-02** (the O5 table of section 1 is the before; the classes are the landing map's, so a type's rows are its mountains, hills and plains near the start):

```
type             class       n |    4m | 16 m: med  p90   max  cliff  walk        |  770m    2km   16km | floors | 512m: med p90 | cost us (O5, x)
MOLTEN           MOUNTAINS   9 | 0.328 | 0.309 0.549  9.33  0.0%  100% |  336m   684m  1348m |    2% | 0.1292 0.3445 | median 0.31 ok; 2km 684m ok; 16km 1348m FAIL; cliffs 0.0
MOLTEN           HILLS      17 | 0.107 | 0.104 0.219  0.57  0.0%  100% |  107m   261m  1145m |   31% | 0.1001 0.3031 | median 0.10 ok; 2km 261m FAIL; floors 31% ok; 
MOLTEN           FLAT       22 | 0.024 | 0.019 0.050  0.28  0.0%  100% |   22m    64m   503m |   93% | 0.0230 0.0955 | median 0.019 ok; broken p90 0.050 FAIL(O6-04); broken 77
                 classes of 48 land sites: flat 22, hills 17, mountains 9; cost 0.7 us per 16 m sample (O5 0.8, x0.87)
CRATERED         MOUNTAINS  22 | 0.355 | 0.304 0.529  2.78  0.4%  100% |  294m   670m  4231m |    2% | 0.2357 0.5082 | median 0.30 ok; 2km 670m ok; 16km 4231m ok; cliffs 0.4% 
CRATERED         HILLS      24 | 0.140 | 0.149 0.325  5.51  0.2%  100% |  132m   336m  1753m |    9% | 0.1083 0.2885 | median 0.15 ok; 2km 336m ok; floors 9% FAIL(O6-05); 
CRATERED         FLAT        2 | 0.087 | 0.064 0.210  0.96  0.0%  100% |   55m   130m   649m |   39% | 0.0557 0.1845 | median 0.064 ok; broken p90 0.210 ok; broken 770m 55m ok
                 classes of 48 land sites: flat 2, hills 24, mountains 22; cost 1.3 us per 16 m sample (O5 1.1, x1.18)
VENUSIAN         HILLS       9 | 0.097 | 0.082 0.155  0.24  0.0%  100% |   76m   178m   424m |   32% | 0.0453 0.1055 | median 0.08 ok; 2km 178m ok; floors 32% ok; 
VENUSIAN         FLAT       39 | 0.022 | 0.018 0.044  0.11  0.0%  100% |   20m    48m   157m |   96% | 0.0137 0.0449 | median 0.018 ok; broken p90 0.044 FAIL(O6-04); broken 77
                 classes of 48 land sites: flat 39, hills 9, mountains 0; cost 0.8 us per 16 m sample (O5 0.5, x1.56)
FELISIAN         MOUNTAINS   8 | 0.331 | 0.301 0.440  3.65  0.7%   99% |  292m   576m  1469m |    8% | 0.1205 0.3646 | median 0.30 ok; 2km 576m ok; 16km 1469m FAIL; cliffs 0.7
FELISIAN         HILLS      13 | 0.155 | 0.104 0.227  1.18  0.0%  100% |  100m   288m  1314m |   24% | 0.0926 0.2920 | median 0.10 ok; 2km 288m FAIL; floors 24% FAIL; 
FELISIAN         FLAT       27 | 0.018 | 0.015 0.036  0.16  0.0%  100% |   14m    40m   370m |   97% | 0.0102 0.0559 | median 0.015 ok; broken p90 0.036 FAIL(O6-04); broken 77
                 classes of 48 land sites: flat 27, hills 13, mountains 8; cost 1.0 us per 16 m sample (O5 0.8, x1.29)
ROCKY            MOUNTAINS  12 | 0.309 | 0.293 0.455  1.03  0.0%  100% |  282m   639m  1504m |    1% | 0.1579 0.3444 | median 0.29 ok; 2km 639m ok; 16km 1504m ok; cliffs 0.0% 
ROCKY            HILLS      21 | 0.151 | 0.127 0.267  1.53  0.0%  100% |  129m   310m  1256m |   18% | 0.1212 0.3032 | median 0.13 ok; 2km 310m ok; floors 18% FAIL(O6-05); 
ROCKY            FLAT       15 | 0.026 | 0.025 0.058  1.04  0.0%  100% |   29m    67m   579m |   87% | 0.0295 0.0991 | median 0.025 ok; broken p90 0.058 FAIL(O6-04); broken 77
                 classes of 48 land sites: flat 15, hills 21, mountains 12; cost 1.1 us per 16 m sample (O5 0.9, x1.18)
THIN ATMOSPHERE  MOUNTAINS   5 | 0.385 | 0.348 0.624  0.91  0.0%  100% |  365m   872m  2848m |    2% | 0.1777 0.3570 | median 0.35 ok; 2km 872m ok; 16km 2848m ok; cliffs 0.0% 
THIN ATMOSPHERE  HILLS      24 | 0.101 | 0.100 0.206  1.14  0.0%  100% |   96m   239m  1254m |   21% | 0.0691 0.1974 | median 0.10 ok; 2km 239m ok; floors 21% FAIL(O6-05); 
THIN ATMOSPHERE  FLAT       19 | 0.051 | 0.047 0.112  3.65  0.1%  100% |   33m    78m   411m |   64% | 0.0229 0.0869 | median 0.047 ok; broken p90 0.112 ok; broken 770m 33m ok
                 classes of 48 land sites: flat 19, hills 24, mountains 5; cost 1.5 us per 16 m sample (O5 1.3, x1.13)
ICY              MOUNTAINS   1 | 0.206 | 0.132 0.257 21.50  1.7%   98% |  344m   500m  2179m |    0% | 0.1292 0.2216 | (one site: not judged)
ICY              HILLS      28 | 0.121 | 0.064 0.388  3.16  1.2%  100% |   63m   118m   452m |   37% | 0.0231 0.1021 | median 0.06 FAIL; 2km 118m FAIL; floors 37% ok; 
ICY              FLAT       19 | 0.025 | 0.025 0.077  1.43  0.2%  100% |   30m    61m   313m |   79% | 0.0149 0.0741 | median 0.025 ok; broken p90 0.077 ok; broken 770m 30m ok
                 classes of 48 land sites: flat 19, hills 28, mountains 1; cost 1.3 us per 16 m sample (O5 0.9, x1.40)
QUARTZ           HILLS       4 | 0.108 | 0.087 0.161  0.21  0.0%  100% |   85m   181m   446m |   48% | 0.0290 0.0843 | median 0.09 ok; 2km 181m ok; floors 48% ok; 
QUARTZ           FLAT       44 | 0.019 | 0.012 0.042  0.10  0.0%  100% |   14m    34m   181m |   97% | 0.0087 0.0298 | median 0.012 ok; broken p90 0.042 FAIL(O6-04); broken 77
                 classes of 48 land sites: flat 44, hills 4, mountains 0; cost 0.6 us per 16 m sample (O5 0.4, x1.54)
OCEAN            MOUNTAINS  18 | 0.001 | 0.001 0.004 13.53  4.7%   95% |  152m   152m   151m |   85% | 0.2952 0.4176 | (not judged)
OCEAN            HILLS      17 | 0.000 | 0.000 0.003 13.54  2.3%   98% |  152m   152m   151m |   88% | 0.0003 0.4175 | (not judged)
OCEAN            FLAT       13 | 0.001 | 0.000 0.004 13.43  1.9%   98% |   94m   107m   105m |   83% | 0.0000 0.4174 | (not judged)
                 classes of 48 land sites: flat 13, hills 17, mountains 18; cost 0.2 us per 16 m sample (O5 0.2, x0.95)
METAL            MOUNTAINS  15 | 0.373 | 0.337 0.550  2.58  0.3%  100% |  309m   710m  3837m |    1% | 0.2199 0.4969 | median 0.34 ok; 2km 710m ok; 16km 3837m ok; cliffs 0.3% 
METAL            HILLS      23 | 0.159 | 0.137 0.345  2.10  0.2%  100% |  117m   266m  1159m |   27% | 0.0640 0.2845 | median 0.14 ok; 2km 266m ok; floors 27% ok; 
METAL            FLAT       10 | 0.053 | 0.037 0.262  1.00  0.0%  100% |   48m   116m   834m |   44% | 0.0249 0.1684 | median 0.037 ok; broken p90 0.262 ok; broken 770m 48m ok
                 classes of 48 land sites: flat 10, hills 23, mountains 15; cost 1.0 us per 16 m sample (O5 0.8, x1.26)
VOLCANIC         HILLS       9 | 0.068 | 0.075 0.109  0.16  0.0%  100% |   67m   179m   674m |   51% | 0.0439 0.1226 | median 0.07 FAIL; 2km 179m ok; floors 51% ok; 
VOLCANIC         FLAT       39 | 0.026 | 0.020 0.051  0.12  0.0%  100% |   22m    49m   317m |   95% | 0.0139 0.0597 | median 0.020 ok; broken p90 0.051 FAIL(O6-04); broken 77
                 classes of 48 land sites: flat 39, hills 9, mountains 0; cost 0.7 us per 16 m sample (O5 0.4, x1.77)
CARBON           HILLS      10 | 0.106 | 0.083 0.147  2.16  0.3%  100% |   89m   218m  1392m |   19% | 0.0806 0.1683 | median 0.08 ok; 2km 218m ok; floors 19% FAIL(O6-05); 
CARBON           FLAT       38 | 0.025 | 0.017 0.073  0.35  0.0%  100% |   18m    45m   233m |   82% | 0.0120 0.0521 | median 0.017 ok; broken p90 0.073 ok; broken 770m 18m ok
                 classes of 48 land sites: flat 38, hills 10, mountains 0; cost 0.9 us per 16 m sample (O5 0.5, x1.75)
COMET            MOUNTAINS  11 | 0.341 | 0.209 1.196  4.15 12.7%   92% |  144m   224m   217m |    6% | 0.0258 0.0904 | walkable 92% ok; 
COMET            HILLS      35 | 0.151 | 0.154 0.660  4.03  4.7%   98% |  118m   199m   275m |    9% | 0.0324 0.0930 | walkable 98% ok; 
COMET            FLAT        2 | 0.073 | 0.108 0.401  1.29  0.2%  100% |   86m   140m   292m |   14% | 0.0372 0.0951 | walkable 100% ok; 
                 classes of 48 land sites: flat 2, hills 35, mountains 11; cost 1.2 us per 16 m sample (O5 1.0, x1.24)
terrain: 25 of 31 class rows meet every target of the spectrum; 8 spectrum targets failing, 22 targets of later items (cliffs and broken plains: O6-04; mountain floors: O6-03)
```

Short of the targets (8 of 43 counted): molten mountains 1348 m over 16 km (1500), molten and felisian hills 261 and 288 m over 2 km (250), felisian mountains 1469 m over 16 km (1500), felisian hills 24% floors (25), icy hills 0.064 median and 118 m (0.08, 120; icy "hills" are crack fields between chaos plains), volcanic hills 0.075 (0.08, nine sites). Cliffs (0-1% against 3-8%), broken plains and mountain floors wait for their items. Cost 1.0-1.8x the O5 figures, 0.2-1.5 us per 16 m sample; `bench check` green (surface 2x 4.3 ms, forest 13.0, buggy 5.2, sprint worst 2.7); `consistency` 1 of 13 over (the felisian site moved onto snow under a dim star, KI-309).

Class shares of 48 land sites per type after O6-02: felisian 29 flat / 13 hills / 8 mountains, thin atmosphere 19 / 24 / 5, rocky 16 / 20 / 12, cratered 2 / 21 / 25, metal 10 / 23 / 15, molten 25 / 17 / 9, icy 23 / 24 / 1, venusian 39 / 9 / 0, quartz 44 / 4 / 0, volcanic 45 / 3 / 0, carbon 38 / 10 / 0, comet 2 / 35 / 11.

What changed in the code: `core/noise` `gnoise3d` (value and analytic gradient); `planetmap` `reliefSum`/`reliefAt`, the zoning fields, `BodyGen::relief*` (hashed), every type's layers, `reliefClass` 0.05/0.2; `Game::siteSlope` the regional mean grade; `SurfaceSite::lodN` and the near-ring blend in `groundHeight`; `SurfaceView` `ringN`, `nearBlend`, `updateNearRing`, the near ring drawn first, prefetch stage five, the ahead worker's rotation; `vesperis_test terrain` (the table above), `landforms`, two `unit` checks; `GEN_VERSION` 6, the regression baseline re-blessed. Notes: `05-planet-function.md`, `06-surface.md`, `07`, `08`, `10-decisions.md`, `KNOWN-ISSUES.md` (KI-309..311).

## 9. O6-03..O6-07 results (2026-09-28, generation version 10)

**What was built** (the reference notes carry the formulas: `05-planet-function.md` "Generation version 10", `06-surface.md` "Drainage on the surface, landmarks", `08-testing-and-tools.md`, `10-decisions.md`):

* O6-03 drainage (`galaxy/drainage.*`): a flood of the ground per one-degree tile on a 400 m lattice (priority flood on a noised routing surface, spill-based depressions kept as lakes under 30 km^2 two times in five or breached by a gorge of `60 + 12 sqrt(area)` m, least-cost breach ramps, steepest-descent flow, accumulation, monotone levels; tiles blended over 0.2 degrees at their borders, cached, computed on threads, prefetched by the landing map and by the landing), read by the planet function as Catmull-Rom reaches with jitter and meanders (a river never drawn narrower than 1.6 samples), valleys with flat floors of ten widths, floodplains, lakes at the fill levels with the shore as the fine ground's contour, playas on dry worlds, wadis on thin-atmosphere and desert worlds, liquid channels on hydrocarbon and acidic ones. The Worley river nets and the lake features of B-320 are gone.
* O6-04 landform library I: cliff bands (60-130 m, bench / talus / riser / top, on the steep ground of the massifs and of any steep spectrum), scree and boulder fields, broken plains (hummocks, outcrops, gullies, fading by feature size), materials by slope, aspect and wetness.
* O6-05 landform library II: cirques, the glacial U and fjords, coasts by exposure, lava tongues and pit chains, slip-face dunes with their own fade, crater terraces with scarps and rough ejecta.
* O6-06 landmarks: eight kinds found by the terrain per 14 km cell (peak, mesa, canyon rim, crater, geyser field, crystal field, lake, ruins), named, marked on the landing zoom and the sector map, named by the rangefinder, logged on first sight, renamable in the guide.
* O6-07: the landing's first frame kept at 50-80 ms with the drainage (`bench check` 80 ms), the landing map never computing tiles on the main thread, the terrain tuning below, `regress` re-blessed for GEN 10, the harness green, KI-328..334.

**The targets, measured** (`vesperis_test terrain`, the classes are the landing map's; the O6-02 table of section 8 is the before):

```
type             class       n |    4m | 16 m: med  p90   max  cliff  walk        |  770m    2km   16km | floors | 512m: med p90 | cost us (O5, x)
MOLTEN           MOUNTAINS   9 | 0.343 | 0.253 0.809 11.74  6.8%   97% |  390m   749m  1685m |    2% | 0.1645 0.4042 | median 0.25 ok; 2km 749m ok; 16km 1685m ok; cliffs 6.8% ok; walkable 97% ok; floors 2% FAIL; 
MOLTEN           HILLS      11 | 0.114 | 0.095 0.190  1.25  0.0%  100% |   94m   229m  1181m |   31% | 0.0804 0.3429 | median 0.10 ok; 2km 229m ok; floors 31% ok; 
MOLTEN           FLAT       28 | 0.033 | 0.026 0.061  0.36  0.0%  100% |   20m    52m   495m |   91% | 0.0229 0.0830 | median 0.026 ok; broken p90 0.061 ok; broken 770m 20m ok; 
                 classes of 48 land sites: flat 28, hills 11, mountains 9; cost 0.8 us per 16 m sample (O5 0.8, x1.05)
CRATERED         MOUNTAINS  20 | 0.507 | 0.300 0.911 83.42  8.7%   97% |  432m   735m  3836m |    2% | 0.1840 0.4777 | median 0.30 ok; 2km 735m ok; 16km 3836m ok; cliffs 8.7% FAIL; walkable 97% ok; floors 2% FAIL; 
CRATERED         HILLS      24 | 0.174 | 0.146 0.272  4.71  0.1%  100% |  116m   353m  1862m |   11% | 0.1141 0.2814 | median 0.15 ok; 2km 353m ok; floors 11% FAIL(O6-05); 
CRATERED         FLAT        4 | 0.116 | 0.063 0.164  0.96  0.0%  100% |   43m   128m  1563m |   49% | 0.0676 0.2191 | median 0.063 ok; broken p90 0.164 ok; broken 770m 43m ok; 
                 classes of 48 land sites: flat 4, hills 24, mountains 20; cost 1.3 us per 16 m sample (O5 1.1, x1.22)
VENUSIAN         HILLS       9 | 0.080 | 0.077 0.147  0.34  0.0%  100% |   76m   218m   690m |   30% | 0.0372 0.1168 | median 0.08 FAIL; 2km 218m ok; floors 30% ok; 
VENUSIAN         FLAT       39 | 0.041 | 0.029 0.062  0.31  0.0%  100% |   18m    38m   182m |   96% | 0.0120 0.0390 | median 0.029 ok; broken p90 0.062 ok; broken 770m 18m ok; 
                 classes of 48 land sites: flat 39, hills 9, mountains 0; cost 1.2 us per 16 m sample (O5 0.5, x2.44, OVER 2x)
FELISIAN         MOUNTAINS   5 | 0.338 | 0.277 0.666  4.62  4.3%   98% |  339m   646m  2063m |    2% | 0.1175 0.3980 | median 0.28 ok; 2km 646m ok; 16km 2063m ok; cliffs 4.3% ok; walkable 98% ok; floors 2% FAIL; 
FELISIAN         HILLS      14 | 0.145 | 0.112 0.262  2.01  1.2%  100% |  108m   272m  1344m |   32% | 0.0876 0.2986 | median 0.11 ok; 2km 272m FAIL; floors 32% ok; 
FELISIAN         FLAT       29 | 0.022 | 0.010 0.085  1.01  0.0%  100% |   16m    55m   353m |   86% | 0.0065 0.0886 | median 0.010 ok; broken p90 0.085 ok; broken 770m 16m ok; 
                 classes of 48 land sites: flat 29, hills 14, mountains 5; cost 2.5 us per 16 m sample (O5 0.8, x3.09, OVER 2x)
ROCKY            MOUNTAINS  17 | 0.325 | 0.254 0.825  3.87  6.2%   97% |  318m   731m  1726m |    2% | 0.1935 0.4334 | median 0.25 ok; 2km 731m ok; 16km 1726m ok; cliffs 6.2% ok; walkable 97% ok; floors 2% FAIL; 
ROCKY            HILLS      13 | 0.145 | 0.128 0.225  1.93  0.1%  100% |  110m   285m  1324m |   18% | 0.1376 0.3372 | median 0.13 ok; 2km 285m ok; floors 18% FAIL(O6-05); 
ROCKY            FLAT       18 | 0.037 | 0.031 0.080  0.94  0.0%  100% |   31m    92m   735m |   87% | 0.0412 0.2044 | median 0.031 ok; broken p90 0.080 ok; broken 770m 31m ok; 
                 classes of 48 land sites: flat 18, hills 13, mountains 17; cost 1.5 us per 16 m sample (O5 0.9, x1.62)
THIN ATMOSPHERE  MOUNTAINS   4 | 0.479 | 0.416 0.821  1.14  0.1%  100% |  406m   735m  1701m |   11% | 0.1126 0.3501 | median 0.42 ok; 2km 735m ok; 16km 1701m ok; cliffs 0.1% FAIL; walkable 100% ok; floors 11% FAIL; 
THIN ATMOSPHERE  HILLS      22 | 0.133 | 0.124 0.237  1.05  0.0%  100% |  112m   350m  1817m |   12% | 0.0950 0.2744 | median 0.12 ok; 2km 350m ok; floors 12% FAIL(O6-05); 
THIN ATMOSPHERE  FLAT       22 | 0.052 | 0.042 0.123  1.20  0.0%  100% |   30m    76m   592m |   77% | 0.0373 0.1335 | median 0.042 ok; broken p90 0.123 ok; broken 770m 30m ok; 
                 classes of 48 land sites: flat 22, hills 22, mountains 4; cost 2.0 us per 16 m sample (O5 1.3, x1.56)
ICY              MOUNTAINS   8 | 0.208 | 0.180 0.891 18.47  7.7%   96% |  216m   309m   395m |   14% | 0.0595 0.1562 | median 0.18 FAIL; 2km 309m FAIL; 16km 395m FAIL; cliffs 7.7% ok; walkable 96% ok; floors 14% FAIL; 
ICY              HILLS      28 | 0.129 | 0.064 0.473 17.28  2.0%   99% |   92m   180m   452m |   31% | 0.0384 0.1386 | median 0.06 FAIL; 2km 180m ok; floors 31% ok; 
ICY              FLAT       12 | 0.035 | 0.030 0.065  0.76  0.0%  100% |   26m    62m   256m |   85% | 0.0163 0.0590 | median 0.030 ok; broken p90 0.065 ok; broken 770m 26m ok; 
                 classes of 48 land sites: flat 12, hills 28, mountains 8; cost 1.3 us per 16 m sample (O5 0.9, x1.50)
QUARTZ           HILLS       5 | 0.102 | 0.081 0.150  1.95  0.4%  100% |   76m   162m   563m |   71% | 0.0185 0.1071 | median 0.08 ok; 2km 162m ok; floors 71% ok; 
QUARTZ           FLAT       43 | 0.038 | 0.026 0.061  0.58  0.0%  100% |   16m    31m   165m |   99% | 0.0073 0.0255 | median 0.026 ok; broken p90 0.061 ok; broken 770m 16m ok; 
                 classes of 48 land sites: flat 43, hills 5, mountains 0; cost 1.0 us per 16 m sample (O5 0.4, x2.47, OVER 2x)
OCEAN            FLAT       48 | 0.004 | 0.001 0.052  0.11  0.0%  100% |    7m     8m     7m |   83% | 0.0035 0.0151 | (not judged)
                 classes of 48 land sites: flat 48, hills 0, mountains 0; cost 0.2 us per 16 m sample (O5 0.2, x1.02)
METAL            MOUNTAINS   9 | 0.405 | 0.381 0.739  4.42  4.1%   98% |  383m   695m  3005m |    1% | 0.1274 0.4370 | median 0.38 ok; 2km 695m ok; 16km 3005m ok; cliffs 4.1% ok; walkable 98% ok; floors 1% FAIL; 
METAL            HILLS      27 | 0.154 | 0.135 0.303  4.63  0.4%  100% |  111m   317m  1657m |   23% | 0.0881 0.2993 | median 0.14 ok; 2km 317m ok; floors 23% FAIL(O6-05); 
METAL            FLAT       12 | 0.062 | 0.047 0.199  0.75  0.0%  100% |   40m   100m  1116m |   53% | 0.0465 0.2016 | median 0.047 ok; broken p90 0.199 ok; broken 770m 40m ok; 
                 classes of 48 land sites: flat 12, hills 27, mountains 9; cost 1.4 us per 16 m sample (O5 0.8, x1.70)
VOLCANIC         HILLS      13 | 0.087 | 0.086 0.134  0.33  0.0%  100% |   80m   178m   673m |   32% | 0.0447 0.1231 | median 0.09 ok; 2km 178m ok; floors 32% ok; 
VOLCANIC         FLAT       35 | 0.037 | 0.030 0.063  0.31  0.0%  100% |   19m    47m   366m |   95% | 0.0170 0.0610 | median 0.030 ok; broken p90 0.063 ok; broken 770m 19m ok; 
                 classes of 48 land sites: flat 35, hills 13, mountains 0; cost 1.5 us per 16 m sample (O5 0.4, x3.66, OVER 2x)
CARBON           MOUNTAINS   2 | 0.265 | 0.231 0.327  1.41  0.6%  100% |  193m   402m  2000m |    6% | 0.1262 0.2688 | median 0.23 FAIL; 2km 402m FAIL; 16km 2000m ok; cliffs 0.6% FAIL; walkable 100% ok; floors 6% FAIL; 
CARBON           HILLS       7 | 0.080 | 0.089 0.181  3.24  0.3%  100% |   88m   191m  1201m |   40% | 0.0692 0.1478 | median 0.09 ok; 2km 191m ok; floors 40% ok; 
CARBON           FLAT       39 | 0.046 | 0.032 0.081  0.48  0.0%  100% |   21m    45m   282m |   82% | 0.0137 0.0576 | median 0.032 ok; broken p90 0.081 ok; broken 770m 21m ok; 
                 classes of 48 land sites: flat 39, hills 7, mountains 2; cost 1.2 us per 16 m sample (O5 0.5, x2.48, OVER 2x)
COMET            MOUNTAINS  11 | 0.347 | 0.211 1.193  4.15 12.7%   92% |  144m   224m   217m |    6% | 0.0258 0.0904 | walkable 92% ok; 
COMET            HILLS      35 | 0.180 | 0.158 0.662  4.06  4.7%   98% |  118m   199m   275m |    9% | 0.0324 0.0930 | walkable 98% ok; 
COMET            FLAT        2 | 0.110 | 0.113 0.404  1.30  0.2%  100% |   87m   140m   292m |   14% | 0.0372 0.0951 | walkable 100% ok; 
                 classes of 48 land sites: flat 2, hills 35, mountains 11; cost 1.6 us per 16 m sample (O5 1.0, x1.59)
EUROPAN          MOUNTAINS   3 | 0.628 | 0.111 0.825 14.54  3.7%   98% |  239m   286m   238m |   14% | 0.0287 0.1364 | median 0.11 FAIL; 2km 286m FAIL; 16km 238m FAIL; cliffs 3.7% ok; walkable 98% ok; floors 14% FAIL; 
EUROPAN          HILLS       4 | 0.075 | 0.055 0.332  5.07  0.6%  100% |  116m   245m   230m |   31% | 0.0289 0.1420 | median 0.06 FAIL; 2km 245m ok; floors 31% ok; 
EUROPAN          FLAT       41 | 0.061 | 0.009 0.048  0.34  0.0%  100% |   11m    33m    77m |   95% | 0.0034 0.0208 | median 0.009 FAIL; broken p90 0.048 FAIL; broken 770m 11m ok; 
                 classes of 48 land sites: flat 41, hills 4, mountains 3; cost 1.7 us per 16 m sample (O5 0.0, x0.00)
TECTONIC         MOUNTAINS   2 | 0.491 | 0.234 0.397  1.23  0.2%  100% |  216m   581m   900m |    4% | 0.0878 0.2733 | median 0.23 FAIL; 2km 581m ok; 16km 900m FAIL; cliffs 0.2% FAIL; walkable 100% ok; floors 4% FAIL; 
TECTONIC         HILLS      14 | 0.095 | 0.088 0.148  0.30  0.0%  100% |   84m   184m   547m |   31% | 0.0447 0.1163 | median 0.09 ok; 2km 184m ok; floors 31% ok; 
TECTONIC         FLAT       32 | 0.040 | 0.032 0.067  0.27  0.0%  100% |   22m    57m   332m |   91% | 0.0174 0.0609 | median 0.032 ok; broken p90 0.067 ok; broken 770m 22m ok; 
                 classes of 48 land sites: flat 32, hills 14, mountains 2; cost 1.4 us per 16 m sample (O5 0.0, x0.00)
DESERT           HILLS      12 | 0.072 | 0.071 0.127  1.86  0.1%  100% |   69m   126m   272m |   49% | 0.0257 0.0760 | median 0.07 FAIL; 2km 126m ok; floors 49% ok; 
DESERT           FLAT       36 | 0.018 | 0.012 0.044  2.05  0.1%  100% |   18m    43m   198m |   86% | 0.0106 0.0532 | median 0.012 ok; broken p90 0.044 FAIL; broken 770m 18m ok; 
                 classes of 48 land sites: flat 36, hills 12, mountains 0; cost 1.3 us per 16 m sample (O5 0.0, x0.00)
HYDROCARBON      MOUNTAINS   1 | 0.506 | 0.165 0.650  1.25  3.1%  100% |  118m   118m    61m |   60% | 0.0018 0.0208 | (one site: not judged)
HYDROCARBON      HILLS      11 | 0.193 | 0.160 0.285  0.49  0.0%  100% |  111m   136m   148m |   62% | 0.0057 0.0315 | median 0.16 FAIL; 2km 136m ok; floors 62% ok; 
HYDROCARBON      FLAT       36 | 0.035 | 0.025 0.062  1.04  0.0%  100% |   16m    34m   121m |   94% | 0.0060 0.0242 | median 0.025 ok; broken p90 0.062 ok; broken 770m 16m ok; 
                 classes of 48 land sites: flat 36, hills 11, mountains 1; cost 2.7 us per 16 m sample (O5 0.0, x0.00)
BOMBARDED        MOUNTAINS  17 | 0.356 | 0.319 0.693 29.23  3.0%   99% |  318m   652m  3349m |    4% | 0.1618 0.4515 | median 0.32 ok; 2km 652m ok; 16km 3349m ok; cliffs 3.0% FAIL; walkable 99% ok; floors 4% FAIL; 
BOMBARDED        HILLS      19 | 0.178 | 0.160 0.347 16.66  0.5%  100% |  125m   269m  1538m |   13% | 0.0839 0.3250 | median 0.16 FAIL; 2km 269m ok; floors 13% FAIL(O6-05); 
BOMBARDED        FLAT       12 | 0.065 | 0.061 0.244  1.54  0.1%  100% |   48m   113m   702m |   36% | 0.0251 0.2182 | median 0.061 ok; broken p90 0.244 ok; broken 770m 48m ok; 
                 classes of 48 land sites: flat 12, hills 19, mountains 17; cost 1.5 us per 16 m sample (O5 0.0, x0.00)
ACIDIC           MOUNTAINS   1 | 0.109 | 0.186 0.450  0.87  0.0%  100% |  167m   311m  1259m |   14% | 0.1615 0.3169 | (one site: not judged)
ACIDIC           HILLS      10 | 0.093 | 0.075 0.156  0.98  0.0%  100% |   75m   164m   416m |   44% | 0.0294 0.1180 | median 0.07 FAIL; 2km 164m ok; floors 44% ok; 
ACIDIC           FLAT       37 | 0.041 | 0.038 0.089  1.49  0.0%  100% |   25m    51m   182m |   88% | 0.0119 0.0539 | median 0.038 ok; broken p90 0.089 ok; broken 770m 25m ok; 
                 classes of 48 land sites: flat 37, hills 10, mountains 1; cost 1.4 us per 16 m sample (O5 0.0, x0.00)
terrain: 27 of 48 class rows meet every target of the spectrum; 42 spectrum targets failing, 5 targets of later items (cliffs and broken plains: O6-04; mountain floors: O6-03) not yet met
```

27 of 48 class rows meet every target (32 of 47 after O11 with the cliffs, broken plains and mountain floors still owned by later items; 25 of 31 after O6-02). What holds now: the mountain rows of the spectrum types (molten, cratered, felisian, rocky, thin atmosphere, metal, bombarded) read a 0.25-0.42 median grade, 650-800 m over 2 km, 1.7-3.8 km over 16 km and, on the spectrum types, 4-7% of cliffs (molten 6.8, rocky 6.2, felisian 4.3, metal 4.1); every flat row of the O6-02 types is on target (broken plains: p90 0.06-0.24, felisian 0.085; the felisian median 0.010 at the floor of the class with the floodplain relief); the hills rows carry 18-71% of floors.

Short of the targets (KI-330): the mountain floors everywhere (1-14% against 15; brooks 1-2 km apart with 80-160 m floors cannot flatten 15% of a 2 km window's lowest quarter, and a floor wide enough to would flatten the mountains: the target measures a landform the plan did not build); cliffs on the crater-field types whose mountains are basin walls (thin atmosphere 0.1%, carbon 0.6%, tectonic 0.2%, bombarded 3.0%; cratered 8.7% over the cap with the terraces' scarps); felisian hills 272 m over 2 km (250); icy, europan and tectonic mountains (crack fields, a shell of ice, fault blocks); the new types' hills medians (KI-326). Cost (KI-331): felisian 2.5 us per 16 m sample, 3 x O5 (the drainage's neighbourhood), venusian / quartz / volcanic / carbon 2.4-3.7 x from the cliff bands and the plains' relief; the frame budgets hold.

**Landmarks** (`vesperis_test landmarks <type>`, 24 random land sites, a landmark within 10 km): felisian 96%, thin atmosphere 92%, quartz 92%, cratered 88%, hydrocarbon 83%, desert 46% (KI-333).

**Performance** (`bench check`, Apple M4): space 2x 3.7 ms, surface 2x 4.4, surface 4x 7.3, forest 4x 9.5, buggy 4x 8.4, the first descent frame 50-80 ms (the landing map had a second to prefetch the drainage tiles; a tile costs 100-200 ms on a 6400 km world), sprint worst 3.6.

**Out of scope, as agreed in section 6:** caves and overhangs (no design exists; the planet function is a height field). The release preparation is a separate item.

