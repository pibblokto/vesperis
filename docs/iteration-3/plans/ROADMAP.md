# Roadmap, third iteration

**Status (2026-09-28): O6 complete (O6-03..O6-07, GEN 10: drainage, the landform libraries, landmarks; the release preparation is the next item, on the user's word). Earlier: O0-O5 done; O6-01 and O6-02 done the same evening and stopped for the user's review of the before/after sheets; the review brought five fixes and improvements (O8, B-307..B-311: the buggy's descent, the companion star flooding the frame, the sun flicker, the picture's resolution, the ground's plates), done the same night; O6-03 next.** Built from a play session: rings unseen (B-301), a shore that could not be reached (B-302), sectors that do not match the landing map (R-301), belts the ship cannot reach (R-302), comets that cannot be landed on (R-303), and the wish for drastically better terrain (`PLAN-terrain-3.md`).

## O0 — Rings

| Id | Item | Size | Depends | Acceptance |
| --- | --- | --- | --- | --- |
| O0-01 | B-301: the ring arc bright on the sky bank (lit face, unlit face, the world's shadow cut out), the parent's rings from a moon the same; the ring profile shared (`StarSystem::ringProfileOf`); the ring's shadow on the ground (`SunInfo::ringShadow`, HUD `RING SHADOW`); the landing map says how high the ring stands (`RINGS 62 deg UP`, `EDGE-ON`, `BELOW THE HORIZON`) and marks ringed sky bodies; rings on 12% of solid planets (generation version 5) | S | - | `moonsky` frames; `unit` ring checks |
| O0-02 | B-305: five-bit banks (32 banks of 2048 intensities); body B gets its own forest/sand/snow/grass banks 16-19, the belt rocks bank 20 | M | - | `unit` bank checks; regress re-blessed |
| O0-03 | B-304: a globe is culled only when its whole sphere is behind the camera plane; with its centre behind, the rays are tested over the whole frame | S | - | `unit`: 2,884 limb pixels with the centre 92 deg off axis |
| O0-04 | B-306: the landing map and its zoom carry the shadows of moons, the parent and the rings, refreshed twice a second | S | - | `unit`: a moon's shadow crosses the map; `landmaps` prints the darkest shadow |
| | **done 2026-09-27** | | | |

## O1 — What you see, you can reach (B-302)

| Id | Item | Size | Depends | Acceptance |
| --- | --- | --- | --- | --- |
| O1-01 | A fourth terrain ring at 2048 m to 53 km, prefetched coarse to fine; haze distances opened (felisian 12 km, airless 45 km) | M | - | `unit` far ring 53 km; bench surface 2x under 16 ms |
| O1-02 | The ground's curvature in `toView` for every object; the floor a curved fan to the horizon | S | O1-01 | frames; regress re-blessed |
| O1-03 | The rangefinder on the HUD (distance, water, minutes on foot or driving) and `M` at the crosshair | S | - | `unit` range 2.3 m at 45 deg down; `flow` prints the range |
| O1-04 | The sector map zooms out further: 512 and 2048 km levels, offered by the world's radius; always centred on you, so the neighbouring sectors are on it as you cross | S | O2-03 | `flow_sectormap_wide.png` |
| O1-05 | The buggy's top speed 180 km/h (50 m/s): torque curve retuned against the drag, gears at 12 and 28 m/s | S | - | `drive` over 150 km/h (169) |
| | **done 2026-09-27** (O1-04/05 later the same day) | | | |

## O2 — Sectors that make sense (R-301)

| Id | Item | Size | Depends | Acceptance |
| --- | --- | --- | --- | --- |
| O2-01 | One sector grid everywhere: 1 x 1 degree cells named `LON:LAT` (`sectorName`); the landing map, the surface HUD and the sector map print it; crossing into another sector on the ground is announced and logged | S | - | `unit` sector names; `flow` HUD frame |
| O2-02 | The landing map's sector zoom (`Z`): a window one sector tall and two wide from the planet function at 64-512 m detail with the sector grid, names, a scale bar; 1 km cursor steps (5 with Shift) | M | O2-01 | `flow_landing_zoom.png` |
| O2-03 | The sector map (`N`) coloured like the landing map (one `mapRampsFor`/`mapColor`), a fourth zoom level of 128 km with the sector grid and names | S | O2-01 | `flow_sectormap.png` |
| | **done 2026-09-27** | | | |

## O3 — Asteroid belts you can fly to (R-302)

| Id | Item | Size | Depends | Acceptance |
| --- | --- | --- | --- | --- |
| O3-01 | Belts as destinations: rows in the analyzer, `L` cycling after the bodies, `Enter` approaches, the ship parks six radii off the biggest rock within two cells of its angle round the star and drifts with it; HUD lines and bracket, mode `IN THE BELT`, the belt's data sheet, `C` refused; saved (`belt` line) | M | - | `flow` belt steps |
| O3-02 | Rocks hashed in co-rotating cells (rings of 400 km turning at their own Kepler rate, `beltRockAt`), sizes mostly under a kilometre, drawn round a ship inside the belt as points or lumpy tumbling meshes (bank 3) | M | O3-01 | `space_belt.png`; `unit` rock identity through the shear; bench belt 2x under 12 ms |
| | **done 2026-09-27** (landing on a rock is out of scope, as agreed) | | | |

## O4 — Comets you can land on (R-303)

| Id | Item | Size | Depends | Acceptance |
| --- | --- | --- | --- | --- |
| O4-01 | `PT_COMET` landable; small bodies (R under 300 km) use the exact sphere in `toView`, rings capped at 1.15 radii, no floor, re-anchor every 0.35 radii | M | O1-02 | `unit` comet checks; `scene comet_day/night` |
| O4-02 | Microgravity: walking and sprinting capped at 0.3 of the escape velocity, the jetpack at 0.35, `Ctrl` thrusts down in the air, no buggy; HUD `GRAV 0.0005 G  ESCAPE 9.2 M/S`, `VENTING 40%`, `CTRL DESCENDS` | S | O4-01 | `flow` comet landing prints |
| O4-03 | The comet's activity on the ground: dust jets from hashed vents on the sunlit side, the coma's haze and the blue ion tail (bank 15) at the anti-solar point, the haze distance cut by the activity, boulders of dirty ice | M | O4-01 | frames |
| | **done 2026-09-27** | | | |

## O5 — The terrain plan (investigation only)

| Id | Item | Size | Depends | Acceptance |
| --- | --- | --- | --- | --- |
| O5-01 | `vesperis_test terrain`: slope and relief statistics of the planet function per type (the numbers of `PLAN-terrain-3.md`) | S | - | stdout table |
| O5-02 | `PLAN-terrain-3.md`: the measured state, the techniques, a milestone breakdown with targets and risks | M | O5-01 | the document |
| | **done 2026-09-27** | | | |

## O6 — Terrain III (see `PLAN-terrain-3.md`, sections 6-9; caves and overhangs are out of scope, the next iteration's) — **done 2026-09-28**, the last content milestone before the release

| Id | Item | Size | Depends |
| --- | --- | --- | --- |
| O6-01 | Landform gallery and metrics as the acceptance harness (`vesperis_test terrain` with pass/fail targets per class, `landforms` galleries and pinned review sites) — **done 2026-09-27** | S | - |
| O6-02 | The spectrum: multifractal amplitudes per type, domain warping, slope-damped octaves (the erosion look), a near ring at 4 m; generation version 6 — **done 2026-09-27**, 25 of 31 class rows on target, sheets in `shots/tests/o6/` | L | O6-01 |
| O6-03 | Drainage: a flood of the ground per one-degree tile on a 400 m lattice (priority flood, spill-based lakes kept by rule or breached by a gorge, least-cost breach ramps, steepest-descent flow, accumulation, monotone levels, tile blending, an LRU of tiles computed on threads and prefetched by the landing map), rivers as Catmull-Rom reaches with meanders and a minimum drawn width, valleys with flat floors, lakes at the fill levels, playas on dry worlds; the Worley nets gone — **done 2026-09-28** | XL | O6-02 |
| O6-04 | Landform library I: cliff bands (a monotone remap of the height on steep ground: bench, talus, riser, top), scree and boulder fields, broken plains (hummocks, outcrops, gullies), materials by slope, aspect and wetness — **done 2026-09-28** (cliffs 4-7% on the mountain rows of the spectrum types; the crater-field types' cliffs are the crater terraces' scarps) | L | O6-02 |
| O6-05 | Landform library II: cirques, glacial U valleys and fjords, coasts by exposure (windward cliffs and platforms, lee beaches and foredunes, barrier islands), lava tongues and pit chains, slip-face dunes with their own fade (KI-311), crater terraces and rough ejecta — **done 2026-09-28** (karst and yardangs were R-305's already) | L | O6-04 |
| O6-06 | Landmarks: six kinds done well (peak, mesa, canyon rim, crater, geyser field, crystal field), placed by the terrain itself, a silhouette at 10-20 km, a payoff up close, marked on the landing zoom and the sector map, named by the rangefinder, a `LANDMARK` log entry on first sight within a kilometre, renamable in the guide; about one within 10 km of 90% of landings — **done 2026-09-28** (eight kinds: peak, mesa, canyon rim, crater, geyser field, crystal field, lake, ruins; `landmarks`: felisian 96% of sites with one within 10 km, thin 92, quartz 92, cratered 88, hydrocarbon 83, desert 46) | M | O6-04 |
| O6-07 | Consistency, performance, generation version 10, docs — **done 2026-09-28**: the landing's first frame at 50-80 ms (the drainage tiles prefetched by the landing map, the far floor and the warm pass on a disc, the 2048 m ring on raw cells, a newer prefetch superseding the older worker's tiles), the landing map never computing tiles on the main thread, the terrain tuning of section 9 of the plan, `regress` re-blessed for GEN 10, every harness mode green (`unit` 0 failures, `flow`, `fuzz 1500`, `drive`, `descent`, `capsule` both sites 0.00%, `landforms sites`, `landmaps`, `seasons`, `input`, `audio`, `settings`, `water 3`, `consistency` 6 of 19 as before, `bench check`), KI-328..334 | M | all |

## O8 — From the O6 review: the buggy, the sun, the picture (2026-09-27)

| Id | Item | Size | Depends | Acceptance |
| --- | --- | --- | --- | --- |
| O8-01 | B-307: the buggy rolls downhill; it leaves the ground only above four g of asked-for downward acceleration, keeps its vertical speed when it does, lands relative to the slope | S | - | `descent`: 0% airborne on a 45% grade |
| O8-02 | B-308: the sun drawn by angle per pixel (space and surface, both suns), the flare's radius clamped, the disc never dimmer than its glow | S | - | `unit`: the companion swing (22.6% ahead, nothing beside or behind) |
| O8-03 | B-309: the visor glint arcs gone, a veiling glare in the framebuffer that fades behind ridges and canopies | S | - | `stability` heat maps |
| O8-04 | B-310: default render scale 4 with the fine 3x3 mush (sheets in `shots/tests/scale_*.png`); band-parallel terrain and flora on a persistent worker pool, the tree list, the spray table, the band-clipped lines; the bench's 4x rows and sections; settings version 2 | M | - | `regress` bit-identical through the parallel drawing; `bench check`: surface 4x 10 ms, forest 4x 14, buggy 4x 9 |
| O8-05 | B-311: bilinear texels, smooth terrain shading by default, two-material cells with noisy contours, the far rings' tiles, tile variants per planet, the broad tone tile | M | - | frames; regress re-blessed |
| | **done 2026-09-27** | | | |

## O9 — From the second O6 review: the sky, the rings, the vegetation, the animals (2026-09-28)

| Id | Item | Size | Depends | Acceptance |
| --- | --- | --- | --- | --- |
| O9-01 | B-312: a globe's box is the exact conic extent of its sphere plus the ring's projected circle (`sphereBox`, `ringBox`) | S | - | `unit`: the limb shows with the centre ten degrees outside the frame, either side, in space and in a sky |
| O9-02 | B-317: sky bodies write their depth in metres | S | - | `unit`: the nearest sky depth lies beyond the far ring |
| O9-03 | B-314: the first moon at 4.5-6 radii and outside the rings, GEN_VERSION 7 | S | - | regress systems re-blessed |
| O9-04 | B-313: geomorphing at every ring edge (the band from the finer ring's cover to the edge), the rings warmed coarse to fine, the overlaps cut to a cell with a depth bias, the near ring at every speed, the buggy's suspension, `groundHeight` on the morphed ring | M | - | `unit`: the rings meet (gap 0.0000 m at every edge; 3 / 17 / 84 / 1015 m unmorphed); `descent`, `drive`; `stability <scene> <m/frame> [scale]` and the scene `felisian_mountains` |
| O9-05 | B-315: leaf clusters with a rasteriser cutout, limbs and bark tiles, banks 16/17, ten silhouettes with a look per planet, prism logs, serrated fronds, the depth test before the texel reads | L | - | `bench check` forest 4x 12.7 ms, 2x 7.7; `shots/tests/vegetation_sheet.png` |
| O9-06 | B-316: habitat cells of 512 m with hashed herds spawned within 900 m and forgotten beyond 1300 m | M | - | `unit`: herds within reach, forgotten behind; `encounters` |
| | **done 2026-09-28** | | | |

## O10 — From the third review: the ground's holes, the flicker, the water, the animals, the variety (2026-09-28)

| Id | Item | Size | Depends | Acceptance |
| --- | --- | --- | --- | --- |
| O10-01 | B-318: a coarser ring skips a cell only when every corner lies under the finer ring (`cellCovered`, `coverOf`) | S | - | `capsule <scene>`: 0.00% sky below the horizon at eight altitudes on the mountain and grassland sites |
| O10-02 | B-319: the leaf cutout and tile fade out on small canopies; `groundHeight` follows lod0's morph onto lod1 | S | - | `stability <scene> 0.1 4`: far-band pops 1.42% -> 1.23% (herd), 1.14% -> 1.00% (grassland); `skyline flips` printed |
| O10-03 | B-321: the felisian climate (greenhouse, Earth's latitude curve, the snow line by the square of the latitude, the base `1500 land^0.7`, the alpine zone from the treeline), `climateTempC` shared with the HUD, habitat chances +33%, the `LIFE` readout | M | - | `climate 30`: 18% ice sheet (26 before), `encounters 300`: 34% of land sites with a herd within 200 m (14), 2.2 habitat herds within 900 m (0.7) |
| O10-04 | B-320: the felisian ground as one function with a smooth ground `hS`; rivers as ribbons cut into it (two networks), lakes as flat-levelled features, pools; the water drawn per vertex and morphed; the Fresnel look; the night ramp | L | - | scenes `felisian_river`, `felisian_lake`; `water 3`; `capsule felisian_river` |
| O10-05 | R-304: thirty-one traits, up to three per body by type; the data sheet's `TRAITS`, the description's phrases; GEN_VERSION 8 | M | - | `traits`: the distribution over 300 bodies |
| O10-06 | R-305: the landform library, eighteen landforms with LOD fades, marks and materials (`MAT_SALT` new) | L | O10-05 | `traits [name]` galleries in `shots/tests/trait_*.png` |
| O10-07 | R-306: the plains' hummocks and gullies, shrubland to 320 m, mounds, tufts to 120 m, outcrops | S | - | scenes `biome_savanna`, `biome_grassland`; `bench check` forest 4x 11.3 ms |
| | **done 2026-09-28** (regress re-blessed for GEN 8 and the look; `unit` 82, `flow`, `fuzz 1500`, `drive`, `descent`, `seasons`, `landmaps`, `input`, `audio`, `settings`, `terrain` felisian rows as before, `consistency` 1 of 13 over as before) | | | |

## O11 — From the fourth review: the water from the water and from the air, six new planet types (2026-09-28)

| Id | Item | Size | Depends | Acceptance |
| --- | --- | --- | --- | --- |
| O11-01 | B-322: the reflections mirror only what stands above the water; the Fresnel grazing clamp; `viewUnderwater`; no crouch afloat | S | - | `spot ... swim` at eye heights 0.5, 0.1, -0.02 m: water, water, underwater |
| O11-02 | B-322: ocean worlds' floes on a shelf sea with an ice foot; the water carries its ring's `zbias`; the capsule not drawn in flight | S | - | `spot ... ascent` 600 / 1500 m: outlines, no cliffs, no line |
| O11-03 | B-322: the shore cut (`SurfaceSample::shore`, `shoreSplitTriangle`, `cacheHeightAt`, the morphed shore, the clamp), banks over the plane (`bankFloor`), the sheet drawn where a corner is wet by its edge | M | - | `capsule felisian_river`: natural edges at eight altitudes; `unit` seams 0.0000 m; KI-321 closed |
| O11-04 | R-307: six types (`PT_EUROPAN`, `PT_TECTONIC`, `PT_DESERT`, `PT_HYDROCARBON`, `PT_BOMBARDED`, `PT_ACIDIC`): planet functions, palettes, `typeSky`, environment, descriptions, traits, the type table (GEN 9), `hasOpaqueDeck`/`isLavaWorld` | L | - | `survey`: 20% of the bodies; `maps`, `landmaps`, `consistency` rows; scenes for each |
| O11-05 | R-307: the events: geysers (europan cracks, geyser basins, tectonic fumaroles), lava fountains, quakes, meteorite strikes, dust devils; `scene <name> [scale] [warm]` stages them | M | O11-04 | scenes `europan_crack`, `tectonic_fissure 2 60`, `bombarded_plain 2 60`, `desert_erg 2 60`; `traits geyser_basins` prints the vents |
| | **done 2026-09-28** (regress re-blessed for GEN 9; `unit` 0 failures with the seams at 0.0000 m, `flow`, `fuzz 1500`, `drive`, `descent`, `landmaps`, `seasons`, `input`, `audio`, `settings`, `water`, `capsule` both sites 0.00%, `bench check` green; `consistency` 6 of 19 over (KI-325), `terrain` 32 of 47 class rows (KI-326)) | | | |

## O7 — Engineering health (continuous)

Every milestone ships with `regress` re-blessed on purpose, `bench check` green, the reference notes updated in place and a `PROGRESS.md` entry.

| Id | Item | Size | Acceptance |
| --- | --- | --- | --- |
| O7-01 | The harness writes to `shots/tests/`; `shots/` keeps only the player's screenshots; the 280 old test frames deleted | S | `ls shots` shows `screenshot_*` and `tests/` only |
| | **done 2026-09-27** | | |
