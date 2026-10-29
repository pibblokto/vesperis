# The planet function

`SurfaceSample sampleSurface(const BodyGen& g, const Vec3& unit, double detailMeters)` is the single source of truth for every body: the orbital map samples it coarsely, the landing code at full detail. **Any change here changes both views**; the LOD gating below keeps them consistent.

The code (E-07): `planetmap.cpp` holds the dispatch (`sampleSurface`, `PlanetMap`); its parts are `planet_bodygen.cpp`, `planet_features.cpp`
(the finders declared in `planet_features.h`), `planet_landforms.cpp`, `planet_felisian.cpp` and the grounds' two files, sharing
`planet_internal.h` (the galaxy's note maps them).

<!-- contents -->
Contents:
- Inputs
- LOD gating
- Feature fields (jittered cubic grids on the sphere)
- Generation version 2 additions (M9, 2026-09-26)
- Per-type layers (summary of generation 5; O6-02 replaced the hill, ridge and detail terms of every row with the relief spectrum, see the last section)
- PlanetMap
- Material palette and exposure (N1, 2026-09-26)
- Consistency rules
- Generation version 3 additions (M5, 2026-09-26)
- Generation version 6: the relief spectrum (O6-02, 2026-09-27)
- Generation version 8: the climate, the water, the traits and the landforms (O10, 2026-09-28)
- Generation version 9: the shelf sea, the water's edge, six new types (O11, 2026-09-28)
- Generation version 10: drainage, the landform library, landmarks (O6-03..O6-06, 2026-09-28)
- B-404 (2026-10-01): one material at every scale
- S-03 (2026-10-02): the glassed worlds of a neutron star
- C-01 (2026-10-02): the dead desert world's seas and rivers
- P-00 (2026-10-21): the ground functions and the kinds' rows
- P-01 (2026-10-21): the furnace worlds' grounds
- P-02 (2026-10-21): the living worlds' grounds
- P-03 (2026-10-21): the locked and the deep's grounds
- P-04 (2026-10-21): the dead worlds' ground in two times
- P-05 (2026-10-22): the far cold's ground
- P-06 (2026-10-22): the strange moons' ground
- P-07 (2026-10-22): the giants' clouds
- Q-01 (2026-10-22): six slots, the facts and eleven landforms
- Q-02 (2026-10-22): the water, the sky, the life and the ground
- Q-03 (2026-10-22): the sounds of the ground
- D-01 (2026-10-23): a drifter's worlds
- D-04 (2026-10-23): the ground in the dark
<!-- /contents -->

## Inputs

* `BodyGen::make(body)` derives all parameters from `body.seed` and `body.type`: seeds `sA..sG`, amplitudes, crater density/depth, sea level (felisian: continent threshold in [-0.35, 0.2]), ice cap latitude, snow line, moisture bias, lava level, fracture width, dune amplitude, cloud cover/scale, boulder density, volcano density, band count, colours (`color`, `color2`, `skyTint`, `vegColor`).
* `unit` is the body-frame unit vector; `p = unit * R` (km) is the noise coordinate, so features are seamless on the sphere.
* `detailMeters` gates frequency content.

## LOD gating

```
octs(wl)         = clamp(floor(log2(wl*1000 / (detail*4))) + 1, 1, 14)   fbm octaves for wavelength wl km
lodFade(wl)      = smoothstep(4, 12, wl*1000 / detail)                     amplitude fade for small features
scaleVisible(c)  = c*1000 >= 3*detail                                       feature grids of cell c km
fb(wl, seed, maxOct) = fbm3(p/wl, seed, min(maxOct, octs(wl))) * lodFade(wl)
rg(...)              = ridged3(...) * lodFade(wl)
featFade(w)      = smoothstep(1.5, 4, w*1000 / detail)                      B-404: a shaped feature of width w km fades by its own width
matVisible(w)    = w*1000 >= 0.25*detail (0.5 on the world map, detail > 2100)   B-404: a material patch of width w km decides the material
```

B-404 (2026-10-01): `lodFade` is for noise (twelve samples per wavelength against aliasing); a shaped feature (a mesa, a tongue, a tower, a trench, a crater's terrace) fades by `featFade` of its own width, full at four samples; and a **material never follows a faded value**: the feature that decides it is read unfaded wherever its patch spans a quarter cell (`matVisible`), so every ring that can draw the patch draws the same one. The slope-based rules read the relief spectrum's *material chain* (`ReliefSum::hM`, `slopeM`, `gradM`: the octaves of 48 m and longer, unfaded at every sampling scale) rather than the ring's own faded slope.

The orbital map uses `detail = 2 pi R * 1000 / 256 * 0.6` (about 110 km for R = 7500 km); LOD0 terrain uses 16 m, LOD1 64 m, LOD2 512 m.

## Feature fields (jittered cubic grids on the sphere)

`visitFeatures(p, R, cellKm, density, seed, reachCells, fn)`: for the 27 cells around `p`, a feature exists if `hash < density`; its centre is a hashed point in the cell, accepted only if within `cellKm/2` of the sphere surface (thin shell, which keeps the set of features consistent for neighbouring samples), then projected onto the sphere; `fn` receives the chord distance and six random numbers.

* `craterField`: radius `r = cell * (0.14 + 0.30 u3)`, depth `r * 0.20 * depthScale * (0.4 + 0.6 u4)`; profile = bowl `-(1 - x^2)` inside, rim `0.35 exp(-((x-1)/0.14)^2)`, ejecta `0.10 exp(-(x-1)/0.35)` outside, central peak for r > 6 km (50%), erosion `0.35 + 0.65 (1 - age)`. Also accumulates `rimBright` and `floorDark` for albedo.
* `domeField`: cosine-squared domes with optional flat top (venusian pancake domes, boulders, quartz mounds).
* `volcanoField`: cones `(1-x)^1.4` with a caldera (sets `inCaldera`, which becomes lava).

## Generation version 2 additions (M9, 2026-09-26)

* **Tectonics** (`tectonicsAt`, felisian, thin-atmosphere, rocky, venusian): plates are worley cells on the unit sphere (`plateK` 0.65-1.05, about 5-12 plates); each plate is continental (45%) or oceanic; the boundary type comes from the hash of the plate pair: convergent (45%: mountain chains from ridged noise at 45 km in a 320 km band; oceanic pairs make island arcs 90 km from the boundary), divergent (30%: continental rifts 700-1500 m deep, mid-ocean ridges), transform (low broken ridges). Plate interiors add +0.32 (continental) or -0.28 (oceanic) to the continent field, so continents are plate-shaped.
* **Erosion look**: hill amplitude fades below 500 m (valleys widen), dry flats (moist < 0.32, low relief) terrace into 60 m mesas with badland ridges, fine detail scales with relief.
* **Rivers and lakes** (felisian): worley cell boundaries at 80 km on land carry rivers when the pair hash < `riverDensity` (0.4-0.9); channel half-width `(0.05 + 0.3 lowness) (0.5 + moist)` km, never below 0.45 x detail (a line at map scale); channels are carved 4-10 m, banks raise moisture by 0.25; water level = the carved height + 5 m x channel. Lakes: land below 600 m with low relief, moist > 0.45 and the 5 km hill field below -0.42, filled to a few metres above the basin floor; frozen (ice, flat) below -2 C. Volcanic islands carry crater lakes. Every water body sets `SurfaceSample::water` (sea 0); the site code renders and swims at that level.
* **Biomes** (`Biome`, `BIOME_NAMES`): temperature index `T = 28 - 0.55 |lat| - 6.5 h/km + tempBias - 6 winter` (tempBias = 0.3 (tempK - 288)); moisture = noise + bias + banks - 0.28 x rain shadow (mountains 90 km upwind); table: ice (< -8), tundra (< 2, or < 10 and dry), taiga (< 10 wet), temperate forest / grassland / desert (< 20 by moisture 0.55 / 0.28), tropical forest / savanna / desert (warmer, 0.55 / 0.35); wetland (moist > 0.75, low, flat), alpine (relief > 0.5 and h > 700). Materials and vegetation density follow the biome; the landing readout and the surface data sheet name it.
* **Craters with history** (`CraterMarks`): rays (young craters, 6-13 bright streaks to seven radii, albedo +0.3), multi-ring basins (r > 60 km, 55%), mare flooding of old large basins (flat floors, basalt), secondary fields around large craters raise the density of the 0.5 km pass.
* **Volcanism**: felisian volcanic islands (0.25 R grid, calderas become lakes), thin-atmosphere shield volcanoes (0.35 R grid, 1400 m) with basalt plains, rocky old shields, icy cryovolcanic domes (25 km grid, bright fresh ice) and geyser spots (2 km grid), volcanic moons (two cone scales, lava in calderas and pools).
* **Dune seas** (`duneField`): the prevailing wind is the body's `windAngle` flipped per hemisphere and turned per 30-degree band; styles transverse, barchan-like or star (`duneStyle`), wavelength `duneWl` 0.12-0.37 km; on thin-atmosphere lowlands (below 200 m) and felisian deserts (moist < 0.28, 5-700 m, flat).
* **Seasons** (`BodyGen::season`, `StarSystem::seasonOf` = dot(spin axis, direction to the star)): the winter hemisphere's cap edge drops 11 degrees, the snow line 900 m; sea ice follows; tundra snows over; `seasonName` gives "NORTHERN SUMMER" etc. `SurfaceSite::init(..., t)` and `SpaceRenderer::setSeason` pass it; maps are regenerated per season step (eight per orbit). B-419: the season is the terrain's alone: what is made on a world (a people's plan, its extent, its histories, its settlements, roads and works, the scanner's sights) reads the planet without it (`unseasoned`: the generator itself where its season is 0, else a copy at 0, at every entry of those readers), so a people's towns stand where they stand whatever the season the explorer came in (a winter's sea ice had counted as land and its snow as ice sheet: nearly every place of a plan moved with the season, a tenth to a fifth of them came and went).
* **New types**: ocean (global sea at -150 m with ice floes as worley plates toward the poles), metal (dark iron, sharp craters, MAT_METAL with speculars), volcanic (cones at two scales, MAT_SULPHUR plains, basalt, lava), carbon (MAT_GRAPHITE plains with diamond (quartz material) patches).
* **Gas giants**: polar vortex pattern beyond 76 degrees in the map; zonal jets (longitude drifts by `0.06 sin(lat bandCount)` rotations) and night-side lightning are applied when the globe is drawn.

## Per-type layers (summary of generation 5; O6-02 replaced the hill, ridge and detail terms of every row with the relief spectrum, see the last section)

| Type | Height layers | Materials / albedo |
| --- | --- | --- |
| Felisian | warped continent fbm (wl 0.6 R) minus sea level, shelf `land*3200` (softened, coastal shelf `-(-h)^1.15`), mountains `rg(0.14 R)^2 * mountainAmp * mask`, ridges `|rg(35 km)| * 900`, hills `fb(5 km) * hillAmp`, detail `fb(0.4 km) * detailAmp` | water (< 0; ice beyond cap+4), snow (above cap or snow line `snowLine - 40 |lat|`), rock (relief > 0.5 and h > 700), sand (beach h < 12+10 hills, or dry moist < 0.28 and lat < 50), forest (moist > 0.55), grass. Vegetation density set for forest/grass. |
| Cratered | `fb(0.5 R) * mountainAmp`, maria = smoothstep(0.15, 0.4, base) flattening, hills, craters at 8 scales (0.55 R .. 0.025 km) with densities 0.5-0.9 | basalt in maria (darker) else dust; rims brighten, floors darken |
| Molten | base fbm, ridged (0.15 R and 25 km), hills, volcano cones at 0.25 R and 30 km, cracks from worley at 12 km | lava where h < lavaLevel or in calderas (glow 1, height clamped), lava cracks (glow 0.75) near the lava level, else basalt |
| Venusian | base fbm, hills, pancake domes (0.08 R and 3 km) | basalt; the orbital view shows clouds only |
| Rocky | ridged creases (0.15 R, 20 km), base, hills, boulders (250 m and 60 m dome grids), craters at 20 km | rock |
| Thin atmosphere | base, ridged mountains with mask, canyons `rg(0.3 R)^9 * 2200`, hills, craters at 5 scales, dunes in lowlands (sin along a fixed direction, `duneAmp`) | ice beyond the cap, rock on high ridges, dust elsewhere (canyons darker) |
| Icy | base, hills, worley fracture networks at 0.15 R, 12 km, 1.2 km (`-60 m * crack`), craters at 3 scales | rock inside cracks (> 0.55) else ice; albedo drops with crack |
| Quartz | base, rounded hills (`round^2 * hillAmp * 3`), domes at 0.06 R, sparse craters | quartz, albedo 0.78 + 0.12 base |
| Gas giant | none | material gas; albedo = latitude bands (three harmonics of `bandCount`) plus turbulence and worley storms, clamped to [0.22, 1]; N5-03: the storm term alone goes to `veg` (GEN_VERSION 4) so the globe can vary it with time |

`sampleCloudPattern(g, lon, lat)`: fbm on the sphere at `cloudScale` with a latitude band term; threshold `1 - cloudCover`; venusian returns a dense swirl pattern.

## PlanetMap

512 x 256 equirectangular texels (M9-15; 256 x 128 before): `height` (float m), `material` (u8), `albedo` (u8 0..255), `cloud` (u8), `veg` (u8). Longitude wraps, latitude clamps. `albedoAt/cloudAt/heightAt` are bilinear; `materialAt` is nearest. Generation takes 40-110 ms, so `SpaceRenderer` runs it on a worker thread per map (`mapIfReady` returns nullptr and the globe is drawn flat until the thread finishes; `mapFor` waits) and caches maps by body seed (up to ~24) with their season step; `PlanetMap::valid`/`generating` are atomic and the object is not copyable.

Consumers: globe rendering, the landing map, cloud cover at a site (`sampleCloudPattern` directly, drifting with `t`), the new-game and test site searches.

## Material palette and exposure (N1, 2026-09-26)

`BodyGen::matColor[MAT_COUNT]` (`materialPalette()`, called at the end of `BodyGen::make`) is the colour of every material on this body without star light, shared by the globe, the landing map and the ground (B-202). Each material mixes a fixed tint with the body hue `Body::color`: rock `lerp(grey-brown, hue, 0.5)`, basalt dark, dust warm, sand tan, grass `lerp(vegColor, straw, 0.4)`, forest `0.85 vegColor`, snow and ice near white (tinted 10-15% by `skyTint` under an atmosphere), water ocean blue (tinted by the sky), lava a fixed hot orange, cloud white, quartz pale, metal grey, sulphur yellow, graphite near black, gas = hue. Per-type overrides: felisian rock and sand take only 15-20% of the (teal) hue; thin-atmosphere worlds take 60-65% everywhere (exotic colours are the type); icy ice is 60% hue and its cracks dark rock `0.42, 0.42, 0.50`; ocean water from `color2`; venusian cloud = hue and warm dim basalt; molten/volcanic/cratered basalt variants; carbon diamond crusts bright; comets bluish ice. Worlds without flora still get a green so every material of every type has a colour (unit test).

Families: `MatFamily` {rock, water, forest, grass, sand, snow}, `matFamily(material)` (sand = sand, dust, sulphur, quartz; snow = snow, ice, cloud; rock = the rest), `familyRep(type, family)` = the material that stands for a family on a type (sand -> sand on felisian, sulphur on volcanic, quartz on quartz/carbon, else dust; snow -> snow on felisian else ice; rock -> basalt on molten/venusian/volcanic/cratered, metal, graphite, gas, else rock).

Exposure: `noonStop(albedo) = 50 clamp(0.45 + 0.75 albedo, 0.5, 1.2)` is the palette stop of a fully lit surface at noon; `exposureStop(albedo, light, atmosphere) = noonStop x light^0.65` (0.55 without atmosphere) with `light` = ambient + (1 - ambient) x Lambert x the star's light factor. The ramp every view uses is `materialRamp(c, skyLight*, day)`: stops at 0 (black), 14 (dark = 0.25c, blended 60% x day toward `0.55 skyLight + 0.12c` when a sky light is given: hemisphere light), 34 (lerp(dark, c, 0.55)), 47 (c), 56 (c -> white 50%), 63 (white); `materialRampColor` evaluates it. `reliefClass(slope)`: FLAT below 0.05, HILLS below 0.2, else MOUNTAINS (rise over run; 0.04 / 0.15 before O6-02).

## Consistency rules

1. Never add a term to the surface path that is not present (at lower amplitude) in the orbital path, and vice versa; use `fb/rg/crater` helpers which gate by `detail`.
2. Material decisions must depend only on values available at both detail levels (height, latitude, noise fields), never on `detailMeters` itself. B-404: nor on a value faded by it; a material reads its feature unfaded under `matVisible` (the patch spans a quarter cell), and a slope through the material chain (`slopeM`). `vesperis_test matlod` measures the rings' agreement, `unit` guards the walked rings'.
3. Sea level is 0 for felisian and ocean worlds; rivers and lakes carry their own level in `SurfaceSample::water`, which the site code reads per vertex (`SurfaceSite::waterAt`).
4. Lava level is per body (`g.lavaLevel`); `SurfaceSite::lavaLevel` mirrors it for molten worlds.
5. Colour comes from `matColor` and the shared ramp and exposure rule in every view (N1); never add a per-view colour table. `vesperis_test consistency` measures the three views against each other.

## Generation version 3 additions (M5, 2026-09-26)

* `BodyGen::locked` / `lockedDir` (M5-08): on felisian worlds the biome temperature gains `45 sub - 6 + 0.35 |lat|` where `sub = dot(unit, lockedDir)` (the sub-stellar point +39 C, the far side -51 C) and moisture loses `0.35 max(0, sub - 0.15)`: deserts under the sun, forests and grass in the twilight belt, snow and frozen seas (`T < -10`) on the night side. Ocean worlds freeze the night side (`nightIce` from `-sub`). Other types are unchanged.
* `PT_SUBSTELLAR` shares the gas giant layer with 3-8 broad bands and darker `color2`; `selfGlow = clamp(L / 0.003, 0.55, 1)` drives the night-side glow in the renderers.
* `PT_COMET` shares the icy layer (small amplitudes, dense craters) so an approached nucleus is a battered icy hill; since O4 it is landable and the surface code treats it as a small body (`06-surface.md`). Its ground is craggy (median slope 0.18 at 16 m, `vesperis_test terrain`).
* `PT_COMPANION` has no map: the space renderer draws it as a sun.

## Generation version 6: the relief spectrum (O6-02, 2026-09-27)

`reliefSum(p, unit, g, a0, ridge, seed, detail)` replaces the fixed-wavelength hill, ridge, badland and 400 m detail terms of every solid type with one self-affine sum: octave i has the wavelength `reliefWl0 / 2^i` (12-20 km, capped at a quarter of the radius) and the amplitude `a0 (wl_i / wl0)^H` with `H` 1.04-1.14 (a little above 1, so the base wavelengths carry the relief and the fine ones the texture), down to the sampling scale. Each octave fades with `lodFade` and its weights depend only on the coarser octaves, so the map, the far ring and the ground add the same octaves at their own scales. Per octave:

* shape `n + (2 (1 - |n|)^2 - 1 - n) ridge_i` with `ridge_i = reliefRidge max(0, 1 - 0.22 i)`: the first octaves are ridged (a sharp crest at the zero crossing, wide valleys: the range and its spurs), the finer ones roll on the flanks; the amplitude is divided by `1 + 1.2 ridge`, since a ridged octave carries about twice the slope of a rolling one, so a type's ridge parameter changes the look and not the grade;
* the erosion look: weight `1 / (1 + reliefErosion slope^2)` with the slope of the sum so far from the analytic gradient (`gnoise3d` in `core/noise`: value and gradient of the same Perlin noise, checked against finite differences by `unit`), so steep ground stops gathering detail while crests and flats gather it;
* the hybrid multifractal (from the second octave): weight `1 + (clamp(0.4 + h / 0.9 a0, 0.08, 1) - 1) reliefHybrid`, low ground smoothed;
* the coordinate folded twice by low-frequency noise (`reliefWarp x wl0` at 1.7 wl0, a third of it at 0.45 wl0) so ridges curve instead of lying in isotropic lumps;
* after the sum a soft sediment floor: below `-0.05 a0` the ground keeps a tenth of its depth (C1 at the floor), the fill of valleys and basins.

It returns metres and the gradient of the sum on the tangent plane as rise over run (`Relief`, `reliefAt` for the tests).

Where it is strong: two zoning fields every solid type reads, `massif = smoothstep(0.2, 0.6, rg(min(110 km, 0.3 R), 3 octaves))`
and `hillsF = smoothstep(-0.15, 0.45, fb(min(0.06 R, 400 km), 3))`. Each type has a mountain-region mask `zone` (felisian:
`mtnMask`, the 1000 km mountain regions and the convergent chains; thin atmosphere: `mtnMask + 0.8 chain`; rocky: the creases and
the chains; molten: `smoothstep(0.45, 0.85, roughL)`; venusian: the highlands and the chains; cratered: `1 - mare`; icy, comet,
quartz, metal, volcanic, carbon: a smoothstep of the base field); `rel = zone x massif` (the massifs are the mountains, the rest
of a region is hill country: `hf = max(hillsF, 0.6 zone)`) and the amplitude is
`reliefAmp = reliefFlat + (reliefHill - reliefFlat) hf (1 - rel) + (reliefMtn - reliefFlat) rel`. The parameters are hashed per
body at the end of `BodyGen::make` (the rng stream is untouched, so the colours and everything else of generation 5 stay). Per 16
km of base wavelength (a comet's are scaled by its capped wavelength): flat 100 m (icy 140, quartz 110), hill 500 m (felisian and
molten 420, cratered 400, icy 600, quartz 340, venusian 550, volcanic 850), mountain 2600 m (felisian 4000, since its ridge blend
`reliefRidge (0.4 + 0.6 rel)` is the softest; molten 3000, rocky 2400, thin atmosphere and metal 2300, venusian and volcanic 1900,
icy 1800, cratered 1500, carbon 1400, quartz 1200, ocean none); `reliefRidge` 0.35-0.75 (rocky +0.2, metal +0.3, molten and thin
atmosphere +0.15, cratered x0.6, icy x0.5, quartz 0.05), `reliefWarp` 0.2-0.45, `reliefErosion` 2.5-5.5, `reliefHybrid` 0.4-0.8
(quartz 0.8). The felisian layer keeps the 5 km `hills` field as a pattern only (beaches, wetlands and the M9-04 lake basins,
where the spectrum is damped by 85% so the draped sheet stays flat until O6-03 fills real basins), the dry-flat terraces, the dune
seas, the volcanic islands and the rivers; `s.relief` (rocks, textures, the alpine biome through `relief0 = rel + 0.25 hf`) is
`relief0 + 0.5 slope`.

Removed with it: felisian `|rg(35 km)| x 900`, `fb(5 km) x hillAmp`, the badlands `rg(1.5 km) x 45`, the unused rain-shadow duplicate `upwind` and the `fb(0.4 km) x detailAmp` term of every type; cratered `fb(20 km)`; molten `rg(25 km)` and `fb(3 km)`; venusian `fb(12 km)`; rocky `rg(20 km)` and `fb(4 km)`; thin atmosphere `rg(30 km)` and `fb(8 km)`; icy and comet `fb(6 km)`; quartz's rounded `fb(3.5 km)`; metal `rg(30 km)` and `fb(4 km)`; volcanic `fb(5 km)`; carbon `fb(6 km)`. Craters, domes, cones, cracks, dunes, tectonics, the continents and the 1000 km mountain regions are as before, so the maps of the airless types are bit-identical (the regression's icy, rocky and cratered map hashes did not move; the thin-atmosphere maps changed through their rock-on-high-ridges rule, and every site profile changed).

Cost: 0.2-1.5 us per 16 m sample on the development Mac, 1.0-1.8x generation 5 (`vesperis_test terrain` prints it against the O5 figures pinned in the harness).

The class: `Game::siteSlope` is the mean of eight directional slopes (150 and 600 m in four directions at the map's 64 m detail) divided by 0.64 (a directional slope averages 2 / pi of the gradient), no longer the steepest probe, so one crack or crater wall no longer makes a flat site "MOUNTAINS"; `reliefClass` cuts at 0.05 and 0.2. The consistency test measures the same grade on the ground's caches and accepts a class flip when the two grades are within 30%.

## Generation version 8: the climate, the water, the traits and the landforms (O10, 2026-09-28)

**The felisian ground as one function** (`felisianGround`, B-320): the continents (`contWl = 0.6 R x contScale`), the plates, the mountain regions, the relief spectrum, the mesas, the islands (`islandDensity`), the shelf and the dunes, returning the ground `h` and the *smooth ground* `hS` = the base plus the relief's octaves of 2 km and longer (`ReliefSum::hCoarse`, with the sediment floor). The continental base is `1500 land^0.7` (B-321; `3200 land^0.8` before put plateaus at 2-3 km) and the mountain amplitude 1500-4000 m. `felisianSample` adds the inland water, the climate and the materials; `lakeLevelAt` evaluates the ground at a lake's centre (100 m detail, one cached entry per thread).

**Climate** (`climateTempC(g, |lat|, h, winter, sub)`, B-321): `T = -18 + 46 cos(lat)^1.5 - 6.5 h/km + tempBias - 6 winter` (+ the locked-world term), `tempBias = 0.3 (tempK + 33 - 288)` on living worlds (the greenhouse; other types keep `0.3 (tempK - 288)`). The snow line is `snowLine (1 - (lat/85)^2) - 900 winter`; the alpine biome begins at `max(700, snowLine/2)` with relief over 0.45; wetlands reach 120 m. The HUD's temperature is this number plus a day-night swing (`-3 + 10 sin(altitude)`). `vesperis_test climate` prints the area-weighted biome shares per world and the cause of the land ice.

**Inland water** (B-320): rivers on the boundaries of 80 km worley cells (`riverDensity`) and of 25 km cells (a third as wide and deep, at 0.8 x the density): half-width `(0.010 + 0.11 lowness)(0.5 + moist)(0.7..1.3)` km, never under 0.45 x the sampling scale; a floodplain to 2.2 half-widths where the fine relief melts into `hS`, a channel to 1.15 half-widths carved `2.5 + 9 lowness min(1, w/0.15)` m, the water at `hS - 1`; `SurfaceSample::water` is set over the whole zone (the plane under the banks) and `waterKind` 1. Lakes (`lakeAt`): features on 7 km cells with `lakeDensity` 0.35-0.8, radius `cell (0.06 + 0.13 u)` km, an outline of three sine wobbles, a bowl whose shore plain sits at the level + 2 m and whose bed goes 2.5-11.5 m under it, the level = the smooth ground at the centre - 2 m where that centre is land over 15 m, relief under 0.4 and moisture over 0.42; `waterKind` 2. Wetland pools (3) where the moisture is over 0.7, the smooth ground 1-120 m, relief under 0.2 and the 5 km hill field under -0.15; rift lakes (4) on the floor of a rift trait. Under the water: `MAT_WATER` (ice under -2 C); within 1.2 m over it: sand bars; the bank's moisture rises by 0.25 x the bank factor.

**Traits** (R-304, `Trait`, `BodyGen::traits[3]`): drawn in `BodyGen::make` from hashes (the rng stream untouched): a count (none 20%, one 40%, two 28%, three 12%), then draws alternating between the type's landform list (60%) and its world list, without repeats, one colour trait, never pangaea with archipelago nor giant with dead flora. The world traits change the parameters (`seaLevel`, `islandDensity`, `contScale`, `lakeDensity`, `riverDensity`, `iceCapLat`, `snowLine`, `tempBias`, `liquidLevel`, `cloudCover`, `vegColor`) and the palette (`materialPalette`: red, black and chalk tints on the ground family; the exotic liquid's colour); the flora and the look read `hasTrait` (giant, luminous, dead, haze). `traitEligible(type, trait)` is the table; `TRAIT_NAMES`, `traitPhrase`, `traitList` the words. S-03 (2026-10-02) adds `TR_GLASSED`, never drawn by the hash: `StarSystem::generate` marks a neutron star's outer survivors (`Body::glassed`) and `BodyGen::make` puts it first, dropping the exotic seas (the glass owns their bank); its landform and material are in the S-03 section below. C-01 (2026-10-02) adds `TR_CIVILISATION`, never drawn by the trait draw either: `BodyGen::make` gives it to one felisian or desert world in eight by a hash of its own (`mix64(seed ^ 0xC1D1)` under 0.125, so no other world's traits move), first among the traits (after the glass), dropping the third; the data sheet says DEAD CIVILISATION, the description ", the ruins of a people that is gone" (a desert's type clause becomes "world of dry seabeds and dead river courses under a dusty sky, salt where its seas were"). Its ruins are the surface's (`06-surface.md`, `peoples/ruins.*`); on a desert world it also sets the old seas (the C-01 section below).

**Landforms** (R-305, `landformsAt(g, p, unit, detail, LandCtx, LandOut)`): called after the relief sum of every solid type (the
felisian one inside `felisianGround`, before the rivers, so the water follows the canyons and rifts) with the context
`{landMask, moist, relief0, h, hS, T}`; returns `dhSmooth` (added to the ground and the smooth ground), `dhFine` (the ground
alone), `flatten`/`flatOffset` (the ground melts into the smooth ground by that fraction, that many metres off it: playas,
glaciers, rift floors), `mat`/`albedo` (applied after the type's own material, never over water or lava; the felisian stage keeps
snow and ice), `glacier`, `riftFloor`, and `mark` (1 where the landform is at its most distinctive, for the galleries:
`SurfaceSample::landform = 1000 mark + metres`). Every landform has a province (`prov(k)`: about a third of the world from a 0.22
R fbm) or a climate mask, and fades with the sampling scale by the width of what it draws (B-404, `featFade`: each canyon by its
width, each mesa cap, karst tower, inselberg, cirque and sinter mound by its diameter, the cinder cones by a kilometre, the chaos
blocks by 4 km and their cracks by 240 m, the trap terraces by their run on the material slope; the periodic ones, the draa, the
yardangs and the polygons, by `lodFade` of their wavelength, KI-311), and decides its material by the width of the patch
(`matVisible`: the canyon walls, the mesa rims, the towers' walls, the risers; the ergs' sand and the great basin at every scale),
so the map carries the rift, the basin, the coronae, the escarpments and the ergs, the 512 m ring the canyons, the big inselbergs,
the chaos blocks, the cones and the terraces of gentle ground, the 64 m ring the mesas, the karst, the geysers and the chaos
cracks, and the 16 and 4 m rings the steep terraces, the polygons, the yardangs, the salt cracks and the spires (KI-320). Before
B-404 every landform faded by `lodFade` of a kilometre figure or its grid cell, which hid the canyons, mesas, inselbergs and cones
from the 512 m ring the note claimed carried them, and its material followed the fade, so the walls and the sand ended at a ring's
edge. The table of the eighteen is in `R-305`. `MAT_SALT` (family snow, bank 8) is the playas' and the sinter's material with its
own meso tile.

**Exotic seas** (`liquidLevel`): after the type's material, ground under the level becomes `MAT_WATER` at that level; `SurfaceSite::hasWater/seaLevel` follow, the look takes the liquid's colour for bank 2.

**The plains' small relief** (R-306, in `felisianGround`): hummocks `0.5 gnoise(130 m) + 0.3 gnoise(60 m)` and gullies on the boundaries of 200 m cells (`-(0.8 + 1.2 moist)` m within 4-7 m of the boundary, where the moisture is over 0.28), both times `smoothstep(0.3, 0.1, relief0) landMask` and their `lodFade`s.

## Generation version 9: the shelf sea, the water's edge, six new types (O11, 2026-09-28)

**Ocean worlds** (B-322): the floe zone (where the ice cover is above zero) lies over a shelf sea, the floor at 6.5 m (`shelf = smoothstep(0, 0.04, cover)`, the open ocean stays at 150 m), and every ice plate rises out of it through a submerged foot: `h = floor + (top - floor) smoothstep(0, 0.13, d)` with `d` the Worley boundary distance in 1.5 km units, the top `0.9 + 0.35 smoothstep(0.13, 0.3, d)`, ice where `h > -0.3`. The plate was a slab at +1 m over a floor at -150 m before: a 151 m wall between two vertices, a stair of cells from the air.

**The water's edge** (B-322, `SurfaceSample::shore`): a river or a lake carries the signed distance to its water line in metres (negative in the water): for a river `(dKm - w (1.15 - 0.65 t)) 1000` with `t` the inverse smoothstep of `1/depth` (where the channel is a metre deep), for a lake `(d - 0.84 r) 1000`; the field is set over the plane's zone and, as `shoreOut`, out to 3.5 half-widths and 1.6 radii, so a bank corner beyond the plane still measures in the same metres as the wet ones. The bank stands over the plane (`bankFloor`: `level + 0.05 + 0.3 min(1, shore/10)` out to the same reach) and the channel lies under it (`level - 0.3`): the outer floodplain's fine relief used to dip under the plane and the plane showed through it in flat patches. The surface cuts every cell that straddles the edge along it (`06-surface.md`).

**Six new types** (R-307, `PT_EUROPAN`..`PT_ACIDIC`, appended after `PT_COMPANION`; the type table's weights change every system): the layers in `sampleSurface`, all with the relief spectrum and the traits' landforms:

* `PT_EUROPAN`: `h = 60 base` plus lineae on the boundaries of 0.25 R, 40 km and 6 km Worley cells (a double ridge `ridge exp(-((d - span)/(0.45 span))^2)` less a trough `exp(-(d/(0.6 span))^2)`, spans 300/110/30 m, ridges 120/45/12 m, faded at 1.5/0.5/0.12 km), a stain within 2.2 spans and a band 2.5-8 km wide along the great ones (`MAT_DUST`, albedo 0.3-0.4; the ice 0.72), chaos rafts (Worley 3 km, +-30 m by hash, where `fb(0.15 R) > 0.35`), craters at 0.1 R and 2 km. Spectrum flat 0.05 / hill 0.3 / mtn 0.5.
* `PT_TECTONIC`: `h = 0.6 mtn base + 700 uplift`, rifts on the boundaries of 0.3 R cells (`smoothstep(11, 7 km, d)`: the floor flat at `hS - 800`), fissures on the rift floors (5 km cells, trenches 30 m deep within 220 m, lava within 50-90 m, fresh basalt to 700 m, sulphur crusts between), volcano fields at 0.2 R and 20 km, ash plains (`MAT_DUST`) where the relief is low; basalt 0.30-0.38.
* `PT_DESERT`: `h = 1400 base` with the tectonic terms, canyons `900 canyon^8` on 0.3 R ridges, mesas (the 45 m terracing of dry flats), basins (`smoothstep(0.1, 0.5, -fb(0.1 R))` below 300-700 m) with ergs (`duneField` at 0.6-1 x `duneAmp` 10-32 m) and salt pans (`MAT_SALT`, flat at `hS - 4`) at their floors, hamada dust elsewhere; craters at 0.35 x the density.
* `PT_HYDROCARBON`: `h = 700 base + 130 - 170 smoothstep(50, 75, |lat|)`, the methane at the reference level (`liquidLevel = 0`: 27% of the surface, the polar lowlands), channels on 30 km cells (25 m deep below 400 m), tholin dunes (`MAT_SAND`) under 35 degrees on low ground, cryovolcanic domes (25 km cells, 180 m), craters at 40 km; the bedrock ice 0.32-0.4.
* `PT_BOMBARDED`: `h = 0.6 mtn base` and craters at six scales (0.3 R, 0.08 R, 8, 1.2, 0.2, 0.04 km; densities 0.5-1.0, depths 0.6-1.0), a dark glassy regolith (`MAT_DUST`, 0.28 + rays 0.5 + rims 0.14).
* `PT_ACIDIC`: `h = 0.8 mtn base + 150 + 600 uplift` with chains and rifts, karst on low flat ground (sinkholes `-9 smoothstep(0.55, 0.15, f1)` on 300 m cells, towers `domeField(1.5 km, 0.25, 70 m)` in the wettest basins), sulphur crusts under 40 m, the acid at the reference level (31%); bleached rock 0.55-0.65.

`typeSky(g, zenith, horizon)` holds the one sky pair per atmospheric type (thin air, venusian, quartz, desert, hydrocarbon, acidic, tectonic, else from `skyTint`), read by the surface's look, the globe's hemisphere light and the landing map. `geyserVents(g, unit, out)` returns the geyser basins' mound centres and radii (the same features `TR_GEYSERS` builds) for the surface's geysers. `hasOpaqueDeck` (venusian, hydrocarbon) and `isLavaWorld` (molten, volcanic, tectonic) in `system.h` are the groups the renderers branch on. Materials: europan stain `(0.55, 0.32, 0.20)`, tectonic basalt `(0.24, 0.21, 0.19)` and ash `(0.50, 0.46, 0.42)`, desert dust/sand/rock ochres, hydrocarbon ice `(0.62, 0.50, 0.32)`, tholin `(0.36, 0.24, 0.12)`, methane `(0.14, 0.10, 0.05)`, the haze deck `(0.85, 0.55, 0.22)`, bombarded dust `(0.46, 0.45, 0.44)`, acidic rock `(0.80, 0.78, 0.60)` and acid `(0.26, 0.32, 0.10)`; S-03: `MAT_GLASS`, pale green `(0.50, 0.66, 0.52)` blended a fifth toward the body's colour. Survey over 2,014 bodies: europan 7.2%, bombarded 4.4%, hydrocarbon 3.2%, desert 2.2%, tectonic 1.8%, acidic 1.3%; materials europan ice 94% / stain 6%, hydrocarbon sea 27%, acidic sea 31%.

## Generation version 10: drainage, the landform library, landmarks (O6-03..O6-06, 2026-09-28)

**Drainage** (`galaxy/drainage.*`): the rivers and lakes of felisian, desert, thin-atmosphere, hydrocarbon and acidic worlds come
from a flood of the ground itself, not from a Worley net. The planet function stays one deterministic function of position; the
drainage is a precomputed layer of it, built per one-degree tile of a world on a lattice of about 400 m (`drainageLatticeN0(R)`
cells per degree, clamped 16-1024) over the tile's core plus a margin of 0.3 degrees, from the planet function sampled at
`max(30, 0.6 x cell)` metres without the drainage (`t_noDrain`). Per tile (`buildTile`, 100-200 ms on a 6400 km world, the
sampling on threads): a routing surface `hr = h + gnoise3(2.5 cells) + 0.15 hash` (a flat plain routed on the raw heights gave
dead-straight D8 combs); a priority flood from the sea cells and the domain's edge (every depression filled to its spill, `lvl`
the filled height); the depression components by spill with a `cut` flag when the component touches the domain edge; the keep rule
(a basin under 30 km^2 not cut, with a hash under 0.4 of its pit cell, stays a lake at its spill; the rest is breached:
`L = spill - min(depth - 4, min(60 + 12 sqrt(area km^2), 380))`, so a big basin drains through a gorge of a hundred metres, not a
1.6 km trench); the breach ramps by least cost from the pit to the spill (`0.3 max(0, hc - base) + 0.002 + 0.05 (hr - h + 1.2)`
per step, the ramp `min(hc, v - 0.005)` continued down the spill's descent); steepest-descent flow directions on the carved
surface `hc`; a topological order (by `hc` and pop index) for the accumulation in km^2 and the main upstream neighbour; monotone
levels along the flow (`lv[downstream] = max(min(lv[downstream], lv[cell]), lake ? lv[cell] : h - 400)`). Cells: `lvl`, `acc`,
`dir` (0-7, 8 none), `up`, flags sea/lake/valid. Tiles are cached (`CACHE_CAP` 28, LRU, a mutex and a condition variable: a sample
that needs a tile another thread is computing waits for it), computed by the first sample that needs one or ahead of time
(`drainagePrefetch(g, unit, radius, async)`; the range includes the blend neighbours: `radius + 0.2 deg + 10 cells`; a later
prefetch supersedes an earlier worker's remaining tiles). Tile-border blending: `readCell` blends `lvl` and `acc` with the
neighbour tiles' estimates by `smoothstep(0, 0.2 deg, edge distance)`; lake flags and levels are the own tile's alone (a basin cut
by another tile's domain edge looks like another lake there). F-03 (2026-10-13): the same tiles, byte for byte, in about half the
time (`drain`'s median tile 163 → 96 ms, its worst 3,202 → 177): the routing noise is made beside each cell's sample on the
sampling threads; the topological order is one integer key a cell sorted (the height's bits made monotone over the pop index: a
strict total order, the comparator's permutation); the breach's queue is emptied at once; and the descent after a breach is
computed in one go once it turns back twice running. Every walk that reached its guard (the tile's cell count) had turned back on
itself from its third or fourth step, two cells each the other's lowest neighbour sinking in turn 5 mm a step (520 million steps,
3.1 s, in a tile of lake country); from there they only sink and nothing round them is written, so their last values are the last
two of the same float subtractions, computed a binade at a time (`drainageSink`, checked against the loop in `unit`). Under
`VESPERIS_TRACE` each tile's line carries its sampling and sort times and a hash of its arrays (`tileHash`): a change to the
routing keeps every hash or is a bug. The two cells end about 1.2 km down the routing surface, last in the topological order, so
the water gathered into them is not carried down past the cell they drain to (KI-408, a generation question). And a sample no
longer depends on what its thread asked before (F-03, the user's choice): `drainageAt` keeps the last neighbourhood of reaches a
thread built (`t_neigh`, keyed by the world, the lattice cell, the coarse flag and the brooks), but the reaches were bent with the
east-west metres of the first point asked in the cell (its latitude's) and the coarse list's channels chosen by the first point's
scale; so the ground near some rivers moved by millimetres to centimetres with the order the cells were asked for, the ahead
worker's and the landing map's zoom on the pool included. The neighbourhood is now made from its cell alone (the cell's centre
latitude; the coarse list keyed by its scale): five of the 67 comparison scenes moved once by 1-45 pixels (one or two steps), the
regress kept its 22 hashes, and the rings' fill on the pool draws the same picture on every core count.

`drainageAt(g, unit, detail, moist)` reads the neighbourhood of the point's lattice cell (thread-local, rebuilt when the cell
changes): every channel cell of the 5 x 5 with `acc >= 0.1 a0` (`a0 = channelA0(g)`: 3 km^2 in lake country, 12 on a dry world,
times `0.5 + 2.5 (1 - moist)^2`), and the big rivers of a 13 x 13 second ring (`acc >= 150`, after a cheap look at the own tile's
raw accumulation). Each channel cell contributes one reach: a Catmull-Rom curve through the node points of its upstream, own,
downstream and next cells (`nodeUV`: the cell centre jittered by 0.25 cells by hash and 0.36 by a noise of 4.7 cells) with a
lateral meander swing of up to `min(max(2.2 w, 0.3 cell) flat, 0.4 cell)` by a noise of wavelength `max(10 w, 2.5 cells)` on flat
reaches (grade under 0.4%). Width `w = 4 + 1.2 sqrt(acc)` m, depth `1.5 + 0.35 acc^0.35`. For each reach the nearest point gives
its distance `d`, level, accumulation, half-width `wh = max(w/2, 0.8 detail)` (a sheet drawn on the corners of cells needs a band
1.6 cells wide, or a diagonal channel breaks into blobs), presence
`pres = smoothstep(0.7, 1.3, acc/a0) smoothstep(0.05 detail, 0.15 detail, w)` (small streams fade at coarse detail), a floodplain
zone `smoothstep(2.2 wh, 0.9 wh, d) pres`, and a valley `vz = smoothstep(1, xf, d/Wv)^1.5 x gate` with
`Wv = clamp(25 w, 200 m, 6 cells)`, `gate = smoothstep(0.08, 0.3, acc/a0) smoothstep(0.4 detail, 1.2 detail, Wv)`, and a flat
floor out to `xf Wv`, `xf = min(0.6, clamp(10 w, 30, 350) floorK / Wv)` (`floorK` 0.6-1 by a 1.8 km noise: fills in some valleys,
gorges in others). The channel that shapes the point is the one whose zone is strongest (a brook beside a big river does not hide
the river's plain); the water surface is the reaches' levels blended by `1/(d + wh)^2` (the nearest reach's level alone jumped by
metres across a meander's neck); the valley is the strongest of the valleys. Lakes: bilinear over the four nearest cell centres of
the flood's lake cells, the containing cell deciding (a neighbouring lake's fade reaches in only where the cell's own ground
stands within 3 m of that lake's surface), only cells within half a metre of the chosen level counting; `lakeLevel = level - 0.5`.
From 1000 m of detail up (the 2048 m ring) the neighbourhood is the own tile's raw cells with straight reaches from centre to
centre and only the channels whose valleys can show (`COARSE_DETAIL`); above 2100 m there is no drainage (the world map has no
rivers).

`applyDrainage` (planet_felisian.cpp) puts it on the ground: the valley melts the height (and the smooth ground `hS`) toward the channel's bank height (`valleyLevel + 1`) by `vz`; within 2.2 half-widths the ground goes to `level + 1`, the channel is carved `depth x pres` under it within 1.15, the bank stands over the plane out to 3.5 half-widths (`bankFloor`), the water plane is `level` over the zone with B-322's `shore`; a lake on a wet world (moist over 0.3) is the water at `lakeLevel` with the shore as the fine ground's contour (`sh = max(20 (h - level), 400 (0.06 - lake))`); on a dry world a filled basin is a playa (`MAT_SALT`, flat at `level - 0.5`). The felisian layer adds the glacial U (`uShape = smoothstep(2, -6, T) smoothstep(0.15, 0.4, relief0)`: `vz = max(vz, min(1, 1.6 valley) uShape)`) and the fjord (a valley floor reaching the sea within 5 m keeps going 45 m under it), a floodplain relief of a metre on 90 and 30 m outside the banks (`wetness` over 0.2; the flat class's median grade needs it), the wetter bottomland (`moist += 0.25 bank + 0.15 wetness`). Thin-atmosphere and desert layers carve the same networks dry (wadis of sand, `DA.bed`, dust flats in the basins); hydrocarbon and acidic layers fill them with their liquid where `h < level`. Gone: the Worley river nets and the lake features of B-320, the lake bowls of B-320 (`lakeBowlAt` remains for the bowls that seed depressions, 9-19 m, `scaleVisible(2.0)`).

**Landform library I** (O6-04): `cliffBands(p, g, h, slope, zone, detail)` remaps the height within bands of `cliffStep` (60-130 m per body) where the spectrum's slope is over 0.22-0.32 and the cliff country is on (`zone = max(smoothstep(0.12, 0.4, relief0), 0.7 smoothstep(0.3, 0.5, slope))`): a bench (35% of the rise at 0.29 x the slope), a talus ramp (20% at 1.4 x), the riser (20% at 2.75 x) and a rounded top; a monotone C1 bijection of the band, so the ground is continuous and no band is higher than the next; `cliffAmount` (0.45-0.85, halved on europan, hydrocarbon, quartz and icy worlds, 0 on comets) selects the cliffed share of the steep ground by a 1.2 km noise; a band's run must span 1.5-4 samples (the 16 m and 4 m rings carry the cliffs, the 64 m ring the slope they stand on). `SurfaceSample::scree` (the talus, 0-255) drives the rocks and the boulder fields; a riser is bare rock whatever lies around it. `plainRelief(p, g, mask, gullies, detail)` breaks the plains: hummocks of 3 m on 220 m, 2 m on 120 m, 0.5 m on 50 m, outcrop knobs of 3-7 m on two fifths of the 100 m cells, shallow gullies of 50 m where it rains; features fade by their own size against the sampling scale (`featFade`: full when a feature spans four samples; `lodFade`'s twelve-samples-per-wavelength rule hid every shaped feature), `plainK`/`gullyK` per type. Materials by slope, aspect and wetness on living worlds: the snow line rises 8% on the sun-facing aspect, the bare rock and scree on slopes, `moist` less on the sunny side.

**Landform library II** (O6-05): cirques on cold high ground, the glacial U and the fjords (above); coasts by exposure (the continent field's gradient against the wind: windward cliffs of 8-40 m with a wave-cut platform, lee beaches with a foredune, barrier islands); `lavaFlows()` on molten, tectonic and volcanic worlds (tongues down the slope with a flow material, pit chains); `duneField` with a slip face (a rise to 0.72 of the wavelength, the fall after) and its own `lodFade(duneWl)` (KI-311 closed: no more bars at 64 m); `craterField(..., detail)` with three terraces on the inner wall of big craters (`x` 0.55-1, risers over a sixth of a terrace's run: scarps over 1:1, faded at `lodFade(0.15 r)`) and rough ejecta. `BodyGen::cliffStep`, `cliffAmount`; `SurfaceSample::scree`.

**Landmarks** (O6-06, `galaxy/landmarks.*`): every cell of a grid of about 14 km (`landmarkCellDeg`) holds at most one landmark, placed by the terrain and never dropped on flat noise, in the order crater (the biggest crater of the cell on crater-field types) > canyon rim > mesa (a Worley cap of 1.4 km) > geyser field (`geyserVents` on `MAT_SALT`) > crystal field (2 m samples on quartz and carbon worlds) > lake (a kept lake of the flood: 3 cells and 0.6 km^2 at least, unless a peak of 200 m stands in the cell) > ruins (a 2 km grid, 5% of its cells, unless a peak of 150 m) > peak (90 m over the cell's mean, refined at 64 and 16 m). A landmark has a kind, a place, a radius, a height, a prominence, a hash id (stable wherever it is asked for) and a name from `generateName` ("MOUNT KEIRA", "LAKE LYOOTRU", "RUINS OF PHIOLXICR"). `landmarksNear(g, name, unit, radius, out, buildTiles)` searches the cells round a point (a cell costs about a millisecond of scanning at 500 m plus the lake pass over 20 x 20 lattice cells; the verdicts are cached per cell); with `buildTiles` false the lakes are read from the drainage tiles already built and a verdict read without them is not cached. Cost and counts: `landmarkCellsSearched`, `landmarkScanMs`.

## B-404 (2026-10-01): one material at every scale

* **The rings disagreed on the materials wherever a material was decided by a faded term.** Each ring samples the function at its own cell size and every feature fades by it (`lodFade`), which is right for heights (the geomorph of B-313 slides the relief in) and wrong for a threshold: the europan stain, faded to 0.41 at 16 m, put the 6 km lineae's brown bands in the 4 m ring alone, so they ended 120 m from the explorer and the edge moved with the camera (the report); the 40 km set's ended at 640 m; the lava tongues' basalt was half there at 64 m and gone at 512; the cliff bands' rock followed the fade of the band's run; every landform's walls, rims, risers, sand and salt went with its fade; the bare rock of steep ground read the ring's own slope, steeper in the fine rings. `matlod` over 1.5 million points of 204 land sites: 0.31% of all ground differed between 4 and 16 m, 0.78% between 16 and 64, 4.8% between 64 and 512, 5.4% between 512 and 2048 (europan 4.4% and 3.4%, volcanic 2.6% and 4.9%, desert 30% at 64/512: the sand seas).
* **The rules.** `matVisible(w)`: a material patch of width w km decides the material, unfaded, wherever it spans a quarter of a cell (a narrower line is the coarser ring's surroundings; a wider one a chain of cells with gaps); half a texel on the world map (over 2100 m), where the trap terraces' risers on a 7 km texel came out as dots and the icy worlds' 7-20 km crack lines must stay the net the globe always showed. `featFade(w)`: a shaped feature's relief fades by its own width, full at four samples (O6-04's rule for the plains' knobs, shared now). The material chain of the relief spectrum (`hM`, `slopeM`, `gradM`): the octaves of 48 m and longer, unfaded at every scale, the same erosion and hybrid weights read from the chain's own coarser octaves, so the slope that decides a material is one number in every ring (the 4 m ring's twelve-sample octaves, so the ground underfoot keeps its look); the far rings and the map evaluate seven to nine more octaves a sample for it (the map 0.1-0.2 s more).
* **Per feature**: the lineae stain unfaded within three spans (90/330/900 m), the double ridges by `featFade` of four spans (the 12 m ridges stood at 41% in the 16 m ring); `lavaFlows` tongues and pits by their own width; `crackAt` returns `{h, m}` (faded relief, unfaded material); `cliffBands` splits the band's strength (`on`, scale-free), the relief's fade (the run against the scale) and the material (the riser, a tenth of the run, against a quarter cell); `landformsAt` as the note above; the ergs' sand is a province (sand from orbit too); the playas unfaded wherever one spans a quarter cell; the glacier by the chain's depth, within 512 m; the thin-atmosphere high rock and the felisian rock, scree, dry ridges and snow-line aspect by `slopeM`; the crystal spires (`scaleVisible(0.01)` wanted a 3 m sample: no ring ever drew them, the crystal landmarks stood on bare ground) and the salt cracks (1.7 m) are the 4 m ring's.
* **After**: 0.06%, 0.46%, 3.0%, 3.6%; europan and volcanic walked rings agree exactly; the residuals are KI-341 (lines a cell and a half wide by the drainage's design, lines under a quarter cell, height thresholds on the far ring's smoother ground). `unit` guards the first europan, volcanic, desert and felisian worlds (two sites each: 0.05/0.05, 0.2/0.2, 0.6/5, 0.6/6 percent at 4/16 and 16/64).
* **What moved.** The felisian maps' rock and biome texels (the chain's slope) and the desert maps' ergs (sand from orbit); the other maps are bit-identical (`maps`, the regression's ten: one changed). No scene frame of the regression moved; `terrain` meets the same 25 of 48 class rows (cliffs +0.1-0.6% on the mountain rows; the 16 km relief of the far samples up where the landforms now reach the 512 m ring); the pinned mountain site's sheet is the same ground (cliff 3.3% -> 3.9%, walkable 100% -> 99%). The generation version stays 11: the G/S series' bump, before anyone keeps a save, and the ground underfoot keeps its materials but where the slope rule or a gate flipped a cell. `tests/regress_baseline.txt` re-blessed for the felisian map.

## S-03 (2026-10-02): the glassed worlds of a neutron star

`TR_GLASSED` (`landformsAt`): the blast that made the neutron star melted the surface; the melt pooled on the flats and lows and froze into sheets of pale green glass, the slopes and highs are scorched rock. The mask is `land x prov(19, -0.45, -0.1) x smoothstep(0.25, 0.08, slopeM) x smoothstep(0.5, 0.25, relief0)`: a province of about half the world on the material chain's slope and the relief, so a sheet is a sheet in every ring and on the map (B-404: `unit` measures the same share at 16 and 512 m within a point). Inside it the fine relief melts into the smooth ground (`flatten 0.85 mask`), ripples of half a metre over 50 m and swells of 1.5 m over 250 m are frozen in (relief only, faded by the ring), and where the mask passes a half the material is `MAT_GLASS` at albedo 0.42 (`mark` where it passes 0.8). The craters of the type cut through the sheets afterwards. Shares over the scan's first five glassed worlds: cratered 14% of the land (a small crater-saturated world keeps little flat ground), metal 44, 59 and 57%, rocky 29%. `MAT_GLASS` is family water (bank 2, the secondary) and the glassed palette colours `MAT_WATER` as the glass too, so the globe, the landing map and the ground agree without a bank of its own; the ground materials round the sheets are blended toward a scorched `(0.26, 0.23, 0.21)`. A dark obsidian was tried first and was a black screen: a neutron star of L 0.03 lights its worlds at the surface's 0.62 floor.

## C-01 (2026-10-02): the dead desert world's seas and rivers

A desert world with `TR_CIVILISATION` is post-apocalyptic: its seas stood at `oldSeaM = -120 + 220 u` metres (`BodyGen::make`) and are dry. In the desert layer of `sampleSurface`, after the landforms and the canyons: `hC` is the coarse ground with its canyons (the base, the tectonic terms and the canyons' 900 m, before the relief); `oldSea = smoothstep(oldSeaM + 1.5, oldSeaM - 1.5, hC)` (a band of 3 m of the coarse ground, so the shore is a low scarp on a sloping coast rather than a ramp), `seaDepth = max(0, oldSeaM - hC)`; the bed is a sediment floor `oldSeaM - 7 - 0.35 seaDepth + 0.1 relief` the ground melts into by `oldSea` (a tenth of the relief survives; a canyon under the level was a sound of the sea and is the deepest part of the floor), and the land within 40 m of the coarse ground over the level is pulled 70% to a bench at `oldSeaM + 3 + 0.15 relief`, fading out from 15 to 40 m (the coastal flats of beach ridges, where the people lived). On the bed the mesas' terracing is off, the basins' dunes and salt pans fall to 30%, the canyon's material reading is off. Materials: the bed is `MAT_ROCK` at albedo 0.36 (the silt crust, darker than the hamada's dust, so the old seas are dark basins from orbit), `MAT_SALT` where the sea stood over 420-570 m deep (the last brine), `MAT_SAND` on the land within 6 m of the level (the old beaches); a wadi's bed and a playa keep their own. The rivers ran wetter: `riverDensity` 0.8 at least and `applyDrainage` with moist 0.75 (a denser net), and the drainage's `isSeaSample` reads `oldSea > 0.5` as the sea (`drainage.cpp`; the tile key carries `oldSeaM`), so the flood's outlets are the old shore and every wadi ends there. `SurfaceSample::oldSea` carries the bed's mask to the finders (`scene desert_dead_sea`) and the unit check. The other seven deserts in eight are deserts and nothing more.

## P-00 (2026-10-21): the ground functions and the kinds' rows

`sampleSurface` is a dispatch: a `GroundCtx` (the point, the scale, the sample it fills, the crater marks, the plates where the kind has
them, the relief zones `massif` and `hillsF`, the traits' landforms, the cliffs, the drainage) and the kind's ground function
(`KindDef::ground`: `groundCratered`, `groundMolten`, `groundVenusian`, `groundRocky`, `groundThinAir`, `groundIce` (icy worlds and
comets), `groundQuartz`, `groundOcean`, `groundEuropan`, `groundTectonic`, `groundDesert`, `groundHydrocarbon`, `groundBombarded`,
`groundAcidic`, `groundMetal`, `groundVolcanic`, `groundCarbon`, `groundGiant`; the felisian layer stays `felisianSample`), then the
solid grounds' common end (scree, the riser's rock, the liquid's plane, the landform's material, the exotic sea). Each ground is its
base, then the skeleton every solid ground shared line for line, `GroundCtx::relief` (the relief spectrum of the amplitude it is given,
`hRel` the ground before it; the cliff bands and the plains with the row's `plainK` and `gullyK` unless the ground terraces its own, the
desert's; the traits' landforms, `hS0` the smooth ground they read), then its own features and its material, albedo and relief. The
crater stack is the row's (`KindDef::craters`, summed in order by `craterStack`; the grids marked `sight` are the landmarks' crater grids,
`sightsOfCellUncached` reads the same rows). A new kind shares a ground and gives it its own parameters by its draws, or brings a ground
of its own (`09-cookbook.md`).

`BodyGen::make` reads the row: its draws in order (`GenDraw`: lo + span x uni, a symmetric draw or a constant), the second colour and
the sky tint (a constant, the body's colour scaled, the thin air's formula, or drawn), the flora's colour on a kind with flora, a giant's
globe; the relief's grades (`KindRelief`), the cliff bands' factors; the palette's mixes (`MatMix`) after the shared tints (`tint::` in
kinds.h); the exotic sea's level and colour. `typeSky` is the row's pair (`SKY_TINT`, `SKY_THIN`, `SKY_FIXED`). `familyRep` is the row's
`famRep`; a kind without flora may lend the grass and forest families (banks 10 and 3 on the ground, the globe's own) to two accent
materials of its own (`KindDef::accent`, `matFamilyOn`: the bank a material is drawn in; `matFamily` stays the material's nature, which
the works and the roads read); no kind of 1.3.2 has one, so nothing moved.

The rule kept throughout: an expression of the old code is the same expression over the row's values (a literal and a variable of the
same value give the same bits; `-ffp-contract=on` fuses a multiply into an add within one expression only, so no expression was split
or joined, and a term a kind did not have is skipped, not added as zero where that could move a bit). The proof is in `docs/decisions/log/`.

## P-01 (2026-10-21): the furnace worlds' grounds

Five ground functions of their own and the desert's for the sixth (generation 16; the kinds and where they come from in
`04-galaxy-and-systems.md`). Each is its base, `GroundCtx::relief`, its features and its materials, as the others.

* **`groundMagma` (magma ocean).** A molten world's crust at a gentler grade (cones of 30 km cells, the crater stack). Round the substellar
  point (`BodyGen::lockedDir`; the kind is made only on a locked world and its tilt is taken, so the point stays under the sun) the crust
  melts under the reference level within `seaReachDeg` (60-75 degrees, `GF_SEA_REACH`, the shore wandering five degrees by a noise of a
  quarter radius): `h += (-80 - h) melt`, the melt from 1.5 degrees outside the reach to 4 inside, so high ground stands in the band as
  headlands; under 0 the ground is `MAT_LAVA` at the level, glowing. Rafts of crust in the shore's last ten degrees (worley cells of 700 m,
  45% of them near the shore and none ten degrees in: a flat top 1.5-5.5 m over the lava, an edge that still glows), cracks running with
  lava within 14 degrees of the shore (the molten world's 12 km crack lines). Past the terminator (86-100 degrees) black glass on the flats
  (`MAT_GLASS`, the kind's first accent: bank 2 is the lava), basalt on the relief, and from 105 degrees a frost of condensed rock
  (`MAT_SALT` in the frost's own pale, patches of a noise of 0.12 radii joining toward the antistellar point, so the map shows them).
  Around a 30% glass, 27% lava, 26% basalt and 17% frost.
* **`groundChthonian` (chthonian).** Smooth and heavy: the base at 300-700 m of mountains and a relief spectrum of low grades (1.3 m per
  16 km of base wavelength in the mountains), broad shallow basins 350 m deep (a noise of 0.15 radii) floored with pale dust, wrinkle
  ridges 70 m high along 40 km crack lines (the crust shrank on the core, as Mercury's), regolith patches of a 6 km noise on the plains,
  the iron itself (`MAT_METAL`, the first accent) on the 30 km ridges' crests. Basalt 87%, dust 12% on the map (the crests are under its
  texels).
* **`groundGlass` (glass).** The cratered ground with its maria (from 0.02 of the base up to 0.22, a third of the world) as sheets of black
  glass (`MAT_GLASS` in the water family's bank 2: no sea, no lava; the row's `famRep[FAM_WATER]`, which `lookFor` and the globe's water
  ramp now read), their cooling cracks (350 m cells, 1.5 m) and conchoidal ripples (a 120 m ridged noise, 2 m); flat-topped obsidian domes
  (18 km cells, 130-260 m) and their flows (`lavaFlows` from the domes' cells, twice the thickness); fields of glass beads (`MAT_SAND`, the
  accent, the fire fountains' orange) in a band round the sheets' edges where a noise of 0.08 radii is up; the highlands' regolith with
  its rays. No dunes: an airless world has no wind (the plan's "dunes of glass sand" became the beads). Around 58-78% regolith, 19-26%
  glass, 9-15% beads.
* **`groundSteam` (steam).** The plates' highlands (the base 400 m up, uplift, chains, rifts), the relief, and fields of vents on the high
  ground (a noise of 0.05 radii over 0.2-0.4): a sinter mound (`MAT_SALT`, 4 m, 60-180 m across) on each feature of a 2 km grid at half
  density, `steamVents` giving their centres to the surface's bursts. The drainage wet (scalding rivers, boiling lakes in the hollows);
  the polar seas under the reference level poleward of `iceCapLat` (58-72 degrees, wandering four); mud (`MAT_MUD`) on the low flats
  under 250 m and along the channels and their banks, rock elsewhere. The lowlands' supercritical fog is the row's (`lowFogM`: the fog
  closes as the explorer walks down to the reference level, `computeEnvironment`). Around 58-63% rock, 34-38% mud, 2-4% water.
* **`groundSalt` (salt).** The desert's base and skeleton; the old sea's level is the row's sea level (`GF_SEA_LEVEL`, 100-400 m: here
  metres, not the living world's continent threshold), and under it the floor is a flat bed keeping a tenth of the relief, deeper
  offshore (`oldSea` as C-01's, carried in `SurfaceSample::oldSea`). On it, by depth, the evaporites settle as a drying sea lays them:
  gypsum along the old shores (`MAT_DUST` in the gypsum's cream, the first 40-60 m), halite (`MAT_SALT`) below, potash (`MAT_POTASH`, the
  second accent, red with iron) where the floor lies 650-900 m under the level and the last brine dried; salt domes rising through the
  floor (14 km cells, 160 m, flat-topped) and their glaciers (`lavaFlows` from the domes' cells), pillars along the shallow shores
  (400 m cells, 14 m, narrow), brine pools on the salt deeper than 120 m (`MAT_BRINE`, the first accent, flat a metre under the floor,
  tens of metres across in 15% of 250 m cells), gypsum dunes over the shores (`duneField` within 80 m over the level). The uplands dry:
  wadis down to the floor, playas, rock on the relief, gypsum dust elsewhere. No great basin trait: the salt fills it. Around 35-73%
  salt, 22-40% gypsum, 2-8% potash.
* **The rust world** is `groundDesert` with its own parameters: the rows' draws, a red palette (purple-brown rock, red dust and sand), its
  pale sky; the dry valleys are the drainage's wadis.

New materials: `MAT_MUD` (family sand), `MAT_POTASH` (family snow, the salts'), `MAT_BRINE` (family water, a flat ground like the lava,
not a water plane), each with its map colour, meso tile (the mud's cracked plates and bubble pits, the potash's fine polygons; the brine
none) and names. The accents of P-00 have their first users (the magma ocean's glass, the chthonian's iron, the glass world's beads, the
salt world's brine and potash) and the tectonic world's sulphur takes one (its own yellow on the ground and the globe, as on the map: it
was the ash's grey).

## P-02 (2026-10-21): the living worlds' grounds

Five of the six are the felisian layer (`felisianGround`, `felisianSample`: `livingGround` is the ground function, so the climate, the
biomes, the snow, the seas, the drainage, the flora, the animals and the sounds come with it), each by its row; the lichen world has a
ground of its own on the thin air's (generation 16; the kinds and where they come from in `04-galaxy-and-systems.md`).

* **The rows' climate.** `greenhouseK` as before; `latGradient` the contrast of `climateTempC`'s latitude term round Earth's mean (15 C):
  `L = 15 + (L - 15) x gradient`, read only where it is not 1 (the felisian's bits stand), the snow line's fall with latitude and the
  cirques' scaled by it too; `daySwing` the ground's day-night swing (`computeEnvironment`, the felisian's 10 degrees); `grassBand`
  widens the grass biomes by its moisture on both sides (forest needs `0.55 + band`, desert lies under `0.28 / 0.35 - band`). The seas:
  `seaDepth` scales the sea's depth after the shelf's power (where it is not 1), and a sea with it under 1 has pale fringing reefs at
  2-40 m on its globe and map (the water's albedo). Two draws: `GF_ISLANDS` (the volcanic islands' density; the archipelago trait takes
  the larger of its 0.5 and the kind's) and `GF_CONT_SCALE` (the continents' wavelength).
* **Archean** (`KT_MICROBIAL`). No plants and no animals (no `KT_FLORA` or `KT_FAUNA`: the flora, the bestiary, the insects and the land
  animals stay off). Where a felisian world grows its plants the wetter ground is microbial mats (`MAT_MATS`, the first accent, in the
  world's own hue by a hash: the retinal mats' purple, the carotenoids' orange or the first photosynthesis's olive-bronze) and the drier
  bare (regolith, `MAT_DUST`, or rock); the wetlands, the river banks, the bars and the shore's flats up to 1.5-4.5 m over the sea are mats,
  and along the shore a reef in patches of a few hundred metres (`MAT_STROMATOLITE`, the second accent: the pale carbonate between its
  domes, by an unfaded noise of 220 m so every ring and the map agree). The domes are objects (`SurfaceView::forDomes`, the drawing and
  the colliders): seven to fifteen on a reef cell of 16 m, one on a third of the mats' cells, 0.7-3.2 m across, a lumpy dome a third to
  three quarters as tall, dark with the mats that grow over them (the mats' bank) on the pale flat (Shark Bay's). A green sea, an orange
  haze, warm grey and brown land, small continents and island arcs; boulders stay off the mats' sediment.
* **Hothouse.** No ice cap (`GF_ICE_CAP` 110), the climate's contrast 0.45 (the poles mild, the equator little warmer), a snow line at
  5,000-7,500 m, wet (moisture +0.08-0.30), the sea up, rain from a lower threshold (most afternoons), a humid haze, a narrow day swing;
  giant flora favoured (`favourTrait`: 35% of the worlds with a free slot); no glaciers, frost or snowball among its traits (`KS_FREEZE`).
  Forest and swamp from the coasts to the poles.
* **Superhabitable.** 7,000-9,500 km and a density of 1.1 (two or three Earth masses), 12 K warmer, wetter, a thick air (1.5-3 atm);
  the continents at 0.3-0.45 of the felisian's scale (`GF_CONT_SCALE`) under a higher sea, so the land breaks into archipelagos, more
  volcanic islands, and seas at 0.12 of the depth (half of them under 300 m: wide turquoise shelves, a turquoise water, pale reefs). (A
  floor saturating at 320-600 m was tried first: a flat-bottomed sea just under the shelf's albedo threshold, no shelves.)
* **Steppe.** Small seas (the sea level 0.4-0.65 down), dry (moisture -0.10 to -0.02) with a grass band 0.12 wider: grass to the
  horizon, forest only on the rivers' wet ground, the lakes salt (the drainage's playas under 0.3 of moisture); a day swing of 18 degrees;
  dry clear air; its animals in big herds (`FA_HERDS`).
* **Tundra** (`KT_ICE_SHEETS`). 11 K colder in its air and the contrast 1.1; the cap at 44-52 degrees and over the land poleward of its
  edge an ice sheet (`iceSheetEdgeDeg`: the cap's edge without the season, the ground being the world's): a dome 2.2 km thick at 14
  degrees in (a parabolic profile, the ice's), a terminus cliff 20-55 m high over its first 30 m (Greenland's land margins), the land's
  fine relief buried under it, its ground snow whatever the season and its steep wall bare ice. The cold flats patterned in polygons as
  R-305's patterned ground (40-100 m, wherever the mean is under freezing, not as a trait); sea ice past the cap; glaciers in every
  range (a snow line of 1,500-3,000 m); the taiga's conifers its first silhouette (`floraFamily`); cold grazers (`FA_COLD`: big,
  short-tailed, white coats among them, a giant walker more often, on the tundra, the taiga and the cold grass). In winter the cap's snow
  lies beyond the sheet over the tundra.
* **`groundLichen` (lichen).** `groundThinAir`, then its life by unfaded fields: moss (`MAT_MOSS`, the second accent) in the wet hollows
  (the basins' floors, the dry valleys' beds and banks, the meltwater three degrees short of the ice caps' edges, and the low ground of a
  0.6 km noise), crusts (`MAT_CRUST`, the first accent, the world's hue by a hash: orange, the map lichen's yellow-green or rust red) in
  patches of a 90 m noise on the rock, the gravel and the dark lava, more of them toward the water. No flora and no animals: nothing
  taller than a hand. A thin clear sky (thicker than a thin air's: 0.03-0.15 atm, water melting on a summer noon).

New materials: `MAT_MATS` and `MAT_MOSS` (family grass where they are no kind's accent), `MAT_STROMATOLITE` and `MAT_CRUST` (family rock),
each with its map colour, meso tile (the mats' wrinkled skin and gas blisters, the reef flat's packed heads, the crust's lobed rosettes and
black thalli, the moss's cushions) and names.

## P-03 (2026-10-21): the locked and the deep's grounds

Two of the six are the felisian layer (the terminator world and the snowball, by their rows), the hycean is the ocean's ground by its row,
and three have grounds of their own (`groundEyeball` on the ocean's, `groundAmmonia` on the methane world's, `groundChemotroph` on the
tectonic crust's); generation 16 (the kinds and where they come from in `04-galaxy-and-systems.md`). No new material: the eyeball's and the
snowball's ice and snow, the chemotroph's springs in the salt world's brine (its water family's rep: a flat pool, no water plane), its sinter
the salt (its second accent), its mats the archean's material in its own pale hue.

* **A locked world's climate** (`KT_FIXED_SUN`; the eyeball and the terminator, both `untilted`: the sun stands still over its point).
  M5-08's locked climate takes the row's contrast (`subContrast`, its 45 degrees but the terminator's 55; `climateTempC` reads it from
  `BodyGen`). A fixed sun's kind skips M5-08's drying under the sun for its own, in `felisianGround` where every reader sees it: its
  moisture by its climate at the sea's level, `ringWet` wetter round 8 C (the night's meltwater, the day's air carried over the ring) and
  `dayDry` drier from 15 C to 45; the mean climate its landforms and permafrost read (`T0`) takes its side of the substellar point.
* **Eyeball** (`GR_EYEBALL`). The ocean's sea, open within its reach of the substellar point (`GF_SEA_REACH`, 24-54 degrees, the shore
  wandering by four), floes of 0.6 km scattered over the last three degrees of the open water (the iris, a pale ring from orbit, on the
  ocean's shelf so the plates rise through an ice foot), a band of brash along the ice front, and past it the sea glacier: a cliff of 8-20 m
  over its first 30 m, then unbroken ice rising 0.35 m a degree toward the night, the old floes' seams ridged 3.5 m, sastrugi, blue ice in
  unfaded patches (fewer on the night side). Its temperature on the ground is the fixed sun's height (`reachClimate`: -1.8 C, the sea's
  freezing, at the edge of the reach, 34 degrees per unit of the sun's sine round it), so the open sea is where it is over freezing.
* **Terminator world** (the felisian layer, `KT_ICE_SHEETS` with `sheetByCold`, `daySeasDry`). Little water (its sea level 0.3-0.6 down),
  a drier air's greenhouse (10 K) and the contrast 55: the day side a desert (dunes, mesas, salt), its seas boiled away where the climate at
  the sea's level is over 30-36 C (their floors dry: salt in the deeps, sand on the old shelves; no more the drainage's sea, so the rivers run
  into them and their lakes are playas), the night side under an ice sheet whose edge is the climate's -4 C (wandering by seven degrees,
  full at -29 C: `iceSheetIn`) with a terminus of 20-55 m over its first 80 m, the frozen seas M5-08's; between them the ring, wet, its
  rivers running off the sheet's edge, forest and grass (26% of the first world's ring green), a perpetual sunset. No polar caps (its poles
  lie on the ring), no day swing (`daySwing` 0: the climate is the place's).
* **Hycean** (`GR_OCEAN`, `evenClimate`). The ocean's sea with no floes (the cap at 110 degrees) and no night-side ice on a locked world
  (its hydrogen carries the day round), a long swell, a dark teal sea under a cream-white haze (a fixed sky, the fog at 3.5 km), 20-100 atm.
* **Ammonia** (`GR_AMMONIA`). The methane world's rounded hills of ice and rock, channels and domes (no tholin dunes), lower toward the
  poles where its seas reach furthest; the sea of ammonia water at the reference level (`GF_LIQUID_LEVEL`, never freezing); slabs of ice
  afloat in its shallows (0.5 km plates, a third of them, 0.6 m over the sea, on a shelf at 6 m under the plates' zone), dark silt along the
  shores, ammonia frost over a frost line of 250-750 m that falls toward the poles; rivers to the sea. A yellow-brown haze the sun shows
  through dimly (no deck), a drizzle now and then.
* **Snowball** (the felisian layer, `KT_ICE_SHEETS` with `nunataks`, `seaGlaciers`, `sheetM` 1,300). Frozen to the equator: a greenhouse
  45 K under the equilibrium (the ice's white) and the contrast 0.9, the cap's edge at 4-12 degrees and the land poleward of it under a sheet
  whose surface lies smooth over the continents' base (`hCont`) with the ranges standing out of it as rock (nunataks, their steep flanks
  scoured bare); the seas past the cap's edge glacier ice 6-14 m over the water at a cliff, rising poleward, ridged and scoured blue, with a
  band of floes three degrees wide on the slushy sea before it; the belt of open sea at the equator. Nothing lives on it (no flora or
  fauna: where plants would stand, frost-shattered gravel or bare rock, the rule for a living ground without flora); blizzards (24 kn winds
  under its clouds' snow), the 22-degree halo of the cold clear air.
* **`groundChemotroph` (chemotroph).** The tectonic crust (rifts with their floors, fissures 30 m deep, cones, ash plains) cold under a dim
  star, its fissures' rims mats instead of lava. Along the rifts' floors and in fields of tens of kilometres on the plains (`springField`,
  unfaded), hot springs on a 0.5 km grid (`springVents` visits the same features): a pool of blue water 12-30 m in radius on a low mound of
  sinter (the pool a metre under its rim), sinter to 2.4 radii, pale mats (white, cream or pink by a hash: the sulphur oxidisers, the
  thermophiles) spreading three to six times as far with a lobed edge; the fields' cores geyser basins, sinter and mats in a kilometre's
  patchwork between the springs (the pale web its globe shows); frost in fields of a hundred kilometres on the cold ground, more on the heights
  and toward the poles, none in a basin (its edges ragged at 3 km only where a ring draws them: the map's texels are wider).
* **The sheets' wall.** An ice sheet's bare blue ice is its own terminus wall now (`FelGround::sheetWall`), not wherever the bedrock under it
  was steep (P-02's tundra drew blue stripes over its buried cliffs: fixed here).

## P-04 (2026-10-21): the dead worlds' ground in two times

Two kinds of a living world that died, the frozen felisian and the scorched felisian (`deadKind`: their row's `thenKind` the felisian), on
the felisian layer under a cover of the present's (`KindDef::cover`, `DC_FROZEN` or `DC_SCORCHED`, applied at the end of `felisianSample`).
A dead row's draws, relief, greenhouse and tints are the felisian's (`thenGround` in `kinds.cpp`), so `BodyGen::make` makes the same ground
in both times; its climate (`tempBias`) is its people's time's (`Body::thenTempK`: a felisian draw's own, else a living 265-300 K by a
hash), its present's mean apart (`BodyGen::nowTempC`, the HUD's formula at the sea's level), no season. Its traits are drawn from the
processes that shaped that ground and still show (the felisian's but flora, storms, haze, the freeze and steaming geysers).

* **The then-world** (`thenWorld` in `planetmap.h`: the world in its people's time, a copy with the living kind, without the cover and the
  season, `deadAs` keeping the dead kind; B-419's `unseasoned` for every other world). Every reader of a people's world reads it: the plan
  and its grid, the extent and the land share, the divide, the histories, the settlements' cells and sites, the roads, the works and the
  land's works, the records' sites, the instruments (`organologyOf`), the ruins' landmarks. The drawing reads the world as it is. A culture
  is built as its time's (the family, the paving: the living kind's) and weathered as the present's (`cultureOf`: a scorched world's dry
  wet and deep burial, a frozen world's wet low, the cold keeping what it holds); a site's drifts are the present's (`SiteRead::drift`: sand
  on a dry world, snow on a frozen one) and a harbour on a frozen sea is "on the ice" (`shoreIce`). The heights are the same in both times
  on every dry place (`unit`: every place of six scorched test worlds' and a staged frozen world's peoples), so every town stands where
  it stood; the drainage's tiles are the then-world's (`genKey` keyed by the living kind, `buildTile` routing on the then-world's ground:
  the rivers it had, shared by both times).
* **Scorched felisian** (`DC_SCORCHED`). The old seas' floors dry at their depth, kilometres under the old shores: salt in the deep basins
  (deeper than 1.5-2.1 km), silt on the slopes, pale reef limestone on the warm shelves (`MAT_STROMATOLITE`, its second accent, where the
  then-world's sea was over 17 C and under 110 m), flagged `oldSea`; the rivers' beds sand and gravel, the lakes' playas salt, the pools'
  clay; the old grassland's and wetland's soils cracked clay (`MAT_MUD`, its first accent) where they were wet, dust where not; the old
  forests' floors dust with their density kept in `veg` (the surface's stone forests); the old ice and snow moraine dust, bare rock on the
  steep. Where its present's mean falls under freezing (a white dwarf's world, a cold one of a giant's, a locked one's night side) frost on
  the salt, the dust, the clay and the sand. A thin hot sky, dust storms and devils.
* **Frozen felisian** (`DC_FROZEN`). The seas ice at their level, flagged `oldSea` (water to every wet test still): flat ice with pressure
  ridges 2.5 m along the old floes' seams, snow drifted over it in fields (fewer toward the equator), blue where bare (`MAT_ICE`, its first
  accent); the rivers and lakes ice at their level; the land under the frost of its fallen air (thicker toward the poles), bare dark rock on
  the steep and the cliffs, the dead soil where the frost lies thin (low equatorial ground); the old forests' density kept in `veg` (the
  surface's dead white forests). A black-blue sky of a trace of air, no weather, -150 C or colder.

## P-05 (2026-10-22): the far cold's ground

Six kinds of the cold orbits, each a ground function of its own (`GR_NITROGEN`, `GR_CRYOVOLCANIC`, `GR_DRY_ICE`, `GR_WANDERER`, `GR_BRINE`,
`GR_FROST`), the solid grounds' skeleton under each (`GroundCtx::relief`, the crater stack, the cliffs and plains, the traits' landforms).
`GroundCtx` keeps the last relief's smooth ground and slope (`hSmooth`, `slopeM`: the ground with the coarse part of its spectrum alone, its
fine hollows filled, on the material chain); `CraterMarks::young` marks the floor of a young crater 2 km wide and more (1 at its middle). A
kind's traits are the processes whose landforms fit it: the nitrogen world, the brine dwarf and the frost dwarf take no great basin (its
floor is basalt), the frost dwarf no escarpments (its fresh frost hides them), the wanderer none whose sheets would lie on its snow (no lava
sheets, cinders or sand seas).

* **Nitrogen** (Pluto, New Horizons 2015). A crust of water ice (rock, cratered, its heights and high latitudes frosted with methane, dark
  red tholin by an unfaded field along the low latitudes), blades of methane ice 380 m tall drawn out north to south on the high ground there
  (`ridged3` stretched along the polar axis); a great plain's basin at a hashed place within 40 degrees of the equator (`nitrogenBasinOf`:
  15-25 degrees round, 1.8 km deep, its rim ragged) and in every basin under the plains' level (`seaLevel`, metres) a plain of nitrogen ice
  (`MAT_ICE`, its accent): cells of 26 km on average, domed 50 m at their middles between troughs 70 m deep and dark with the tholin they
  gather, fields of sublimation pits 150-350 m across, blocks of water ice afloat in it (hills 1.3 km high along its shores, a few smaller
  ones further out: rock), the traits' walls buried under it; nitrogen glaciers filling the valleys of its shores flat a few hundred metres
  up from it. A black sky of ten millionths of a bar with the haze's blue layers on the horizon (`KindDef::limbHaze`).
* **Cryovolcanic** (Triton, Voyager 2 1989). A young crust of ice with few craters: the cantaloupe terrain over most of it (an unfaded
  field: dimples 25-35 km across between low ridges and smaller ones in them, their cells warped round, long double ridges crossing them),
  smooth plains where its ice flooded and froze, set lower behind their scarps (blue-grey ice); south of the cap's edge (`iceCapLat` and a
  wander, `cryoCapEdge`) a cap of pink nitrogen frost; on the cap the plumes' vents on cells of 120 km (`plumeVents`, `volcanoes`' share)
  and their dark streaks laid downwind (`prevailingWind`: the world's `windAngle` from the local north), 45-110 km long, widening to 19 km.
* **Dry ice** (Mars' south polar cap). The thin air's ground (`groundThinAir`: canyons, shields, dunes, the old dry valleys) under caps of
  carbon dioxide ice to `iceCapLat` (50-64 degrees) pitted like Swiss cheese (flat-floored pits 150-450 m across, 8 m deep); round each cap a
  band 9 degrees wide of layered dust with the spiders (radial channels 1.6 m deep converging on a centre, 300-800 m across: `springJets`
  visits the same features), and over it in the cold half of the hemisphere's year the seasonal frost (`MAT_SNOW`, its second accent: 12
  degrees past the cap in winter, none by midsummer, by `BodyGen::season`), the dark fans the spring's jets laid on it downwind of the
  spiders' centres (`MAT_BASALT`, its first accent).
* **Wanderer** (a rogue world a star caught). The thin air's ground of the world it was (its mountains, craters and shields, its old river
  courses dry: the drainage) under its air fallen as snow (`MAT_SNOW`): `snowLine` metres (6-16) on the open ground of its lowlands, the
  fine relief's hollows filled near flat to `hSmooth` (the craters' floors, the valleys), thinning to nothing 900 m over the old world's
  middle height (an unfaded field, ragged), drifted into low dunes; bare rock on the steep ground that sheds it and the old dark ground of its
  highlands; domes of new ice 450 m high where its interior's warmth pushed up (`volcanoes`). Airless and black.
* **Brine dwarf** (Ceres, Dawn 2015-18). A dwarf's cratered ground, dark with carbon (albedo 0.14-0.2, the walls rock); on the floors of its
  young craters (`CraterMarks::young`) the faculae, salt dried out of brine welled up from below (a bright floor at the middle of the
  youngest, spots by an unfaded field over the rest); one lone mountain of ice (`loneMountainOf`, hashed within 44 degrees of the equator:
  Ahuna Mons's 3.6-4 km, a top 10 km across with a cracked crown on flanks steep to their foot at 12 km, bright streaks of salt down the
  flanks' fall lines).
* **Frost dwarf** (Eris and Makemake). Smooth plains under a fresh coat of methane frost (albedo 0.94-0.97, the brightest ground there is),
  low rounded relief, few craters, fields of sublimation pits (100-250 m across, 18 m deep, their walls darker), a glaze of nitrogen ice in
  the hollows, and red streaks of tholin drawn out east and west (`fbm3` over the position with its polar coordinate stretched ten times).

## P-06 (2026-10-22): the strange moons' ground

Five kinds of moon, each a ground function of its own (`GR_GROOVED`, `GR_TWO_FACED`, `GR_TIGER`, `GR_PATCHWORK`, `GR_SPONGE`) on the solid
skeleton (`GroundCtx::relief`, the cliffs and plains, the traits' landforms; the crater stack scaled by each kind's own ages of ground). All
airless and black-skied. Their traits are the processes that fit them: no great basin on any (its floor is basalt), no escarpments on the
grooved moon (their walls' lines would cross its lanes); the sponge's pits are all it has.

* **Grooved ice** (Ganymede, Galileo 1996-2000). Polygons of a grid of 0.55 radii (`worley3`, warped a little): the boundaries are lanes of bright
  ice (the sulci, `MAT_ICE`, its accent), each boundary its own half-width (7-18% of the grid), drop (200-500 m), groove spacing (3-8 km) and
  height (300-700 m), both sides alike (`mix64(id1 ^ id2)`); in a lane tilted blocks along it (the distance from the boundary: a scarp over a
  fifth of the spacing, then the long back), a finer set 0.6-1.1 km apart 40-80 m high, and a third of a smaller grid's cells swaths at angles
  of their own; between the lanes the dark old ground (`MAT_DUST`, albedo 0.24-0.3, its craters' rims bright), the arcs of ancient impacts'
  furrows on it (troughs 12-22 km apart round hashed middles); the crater stack two fifths as deep on the lanes; a thin frost brightening
  both poleward of `iceCapLat`, a cap of it (`MAT_SNOW`) past 14 degrees more.
* **Two-faced** (Iapetus, Cassini 2004-07). The face it leads with coal-black (`MAT_DUST`, albedo 0.04-0.06) round its apex (`twoFacedApex`:
  `BodyGen::apexDir`), to about fifty degrees of latitude there and a quarter turn and more round the equator either way, by an unfaded score
  of the apex's cosine, the latitude and two noises (its edge ragged with tongues and bays); where the score is near nought (a band some tens of
  kilometres wide) the runaway's own pattern, the slopes that face the equator dark and those that face the poles bright (the material chain's
  gradient, `ReliefSum::gradM`), the craters' floors dark and their rims bright; the rest bright ice (0.55-0.65), greyer near the dark. A ridge
  along the equator 8-15 km high and 22-32 km across (flanks of 30-40 degrees, a rough crest wandering a few kilometres off the equator),
  continuous within about 130 degrees of the apex and broken into lone peaks beyond; craters at every scale, basins half the radius across.
* **Tiger-striped** (Enceladus, Cassini 2005-15). The whitest ground there is (`MAT_SNOW`, albedo 0.9-0.99): old cratered plains to the north
  and in patches (an unfaded field), young plains elsewhere ridged along a net of 60 km (low ridges 140 m high), the crater stack a fifth as
  deep there. South of `iceCapLat` (52-60 degrees) the south polar terrain, 350 m lower behind a belt of folds along its edge (9 km apart, 220 m),
  and in it the four stripes (`tigerStripesOf`: curves in the pole's tangent plane 0.13-0.16 radii apart and 0.45-0.6 long, bowed a little the
  same way, wandering a little; `stripeDistanceKm`), each a rift 500 m deep and 2 km wide between ridges 160 m high, its ice coarse and blue
  (`MAT_ICE`, its accent) to 4-6 km out, the ground between grooved along them (1.1 km apart, 22 m). The curtains' vents along them, one a
  kilometre (`curtainVents`, EV_CURTAINS: `06-surface.md`).
* **Patchwork** (Miranda, Voyager 2 1986). Blocks of unlike old ground fitted together (a grid of 0.4 radii: each block its own shade,
  roughness, craters and height, which fades to nought at its edge, so two blocks meet in a low scarp); up to three coronae (`coronaeOf`: ovals
  0.26-0.38 radii long, 0.6-0.9 as wide, squared off at the corners, clear of each other; a point's place in one by its gnomonic plane,
  `coronaAt`), each 600 m lower than the old ground, a belt of parallel ridges and troughs following its edge (5-10 km apart, 300-600 m) in
  bright and dark bands (a fifth of the corona's size apart), a rougher core; two chasms along arcs of great circles (graben 12-24 km wide and
  2.5-5 km deep, a shoulder each side, their walls rock); along part of the first corona's edge (half a span of 0.6-0.9 radians round it) the
  great cliff (Verona Rupes, `cliffOf`, `greatCliffOf`): the corona's floor dropped and the old ground lifted by half of 15-20 km each, falling
  back over 70 km, the face a few kilometres across, rock.
* **Sponge** (Hyperion, Cassini 2005). Pits packed edge to edge at five scales (0.18, 0.045, 0.011, 0.0028 and 0.0007 radii), each a twentieth
  to a fifth of its width deep (a share by its own number), its floor flat and its walls steepening to a knife-edged rim on the cells'
  boundaries (`1 - r^3` of `r`, twice the nearest over the nearest and the second nearest); the cells are a grid's features projected onto the
  sphere (`surfaceCells`, `visitFeatures`' own: a `worley3` cuts its cells through the sphere, so its nearest feature is seldom under a point
  and a pit's middle was rare on the ground: a fiftieth had a floor); dark red-brown dust in the floors (`MAT_DUST`, albedo 0.08-0.12) wherever
  the floor spans half a sample, bright walls of ice (0.38-0.5, the rims brightest).

## P-07 (2026-10-22): the giants' clouds

Six kinds of giant on the giant's ground (`GR_GIANT`: bands, storms, a great storm), each its class's (`KindDef::giantClass`) character from
`giantGlobe` by hashes of its seed: the water-cloud giant's storms many (0.12-0.32 of the storm cells: water's convection) under soft bands
(0.45-0.65 of a plain giant's contrast), its bands 0.16 brighter (water's clouds the brightest: a Bond albedo of 0.8), the one class besides the
plain giant with a great storm; the azure giant's faint (0.25-0.4) and nearly stormless (0.01-0.03: no cloud to make them); the alkali giant's few
broad bands (0.35-0.5, three to six), no storm; the silicate giant's patchy clouds of rock (0.5-0.7, storms 0.06-0.16); the gas dwarf's haze
hiding all but a trace of them (0.06-0.12, no storm); the super-puff's faint (0.12-0.22). What the class looks like from afar and from inside
is drawn, not in the ground: the globe's ramps, the hot giants' night glow and the hazy limbs (`02-rendering.md`), the probe's air
(`galaxy/probe.*`). The X-03 ice giant is a drawn gas giant's (`isIceGiant`): an azure giant is blue for another reason.

## Q-01 (2026-10-22): six slots, the facts and eleven landforms

**Six slots** (`BodyGen::traits[TRAIT_SLOTS]`). The first three are R-304's draw as it was: it lists R-304's traits alone (`TraitDef::kind`
`TK_FIRST`; the new rows come after the given ones, `TR_GLASSED` and `TR_CIVILISATION`), so no world's first three moved. The last three are
drawn by hashes of their own (`0x51075`): how many as R-304's (none 20%, one 40%, two 28%, three 12%), each from a kind of trait of its own
(`TraitKind`: landform, water, sky, life, ground, sound) that fits the world: the kinds in an order hashed for the world, each taken that holds a
trait its processes allow and no slot excludes (one colour trait a world; an exclusion holds either way: the penitentes and R-304's spire
fields, whose ice spires are penitentes), the trait from its kind by a hash; packed, `TR_NONE` after them. A world with a people and the home
world take no trait that moves the ground (`TraitDef::moves`): their towns, roads and works and the start's ground stand where 1.3.2 put them.
Q-01 has the landforms alone, so a world a new landform fits takes one about three times in four (the exclusions take a few; Q-02 and Q-03
add the other kinds, which share the three slots). Every reader reads six: `hasTrait`, `traitList`, the sentence, `landformsAt` (R-304's slots end at their first empty one, the
new ones are after them), the drainage's tile key (the new slots only where set: a world without them keeps its key), the sights.

**The eleven landforms** (`landformsAt`; each its own processes, `KS_HOT_SPOT` to `KS_PHREATIC`, given to the kinds in `buildKinds`'s Q-01
block; its province, `prov(20..28)`; its mark, and since Q-01 the trait that set the mark, `LandOut::markTrait`, `SurfaceSample::landformTrait`):
* **Great shield** (Olympus Mons): one volcano (`greatShieldOf`: within 44 degrees of the equator) 300-600 km across on a world of Mars' size
  (0.09-0.17 of a smaller world's radius), 22 km high at Mars' radius and lower as the radius (the gravity) grows (9-25 km): a convex dome of
  lava lobes (dark basalt streaks running down from the summit, a radius' sixtieth wide), a caldera of two to four nested pits 1-3 km deep, a
  cliff ring at its foot a fifth of its height (the basal scarp, rock), the aureole's ridged lobes over the plain to 2.3 radii; out of a sea
  an island. On the thin-aired, rocky, venusian, desert, acidic, rust, dry ice, lichen and archean worlds (no plates to carry the hot spot off).
* **Tesserae** (Venus' Alpha and Ovda Regio): plateaus 1-2.5 km over the plains with steep margins, folded into ridges 8-16 km apart and
  broken across by graben 3-6 km apart, a third of that wide and 200-400 m deep, the two sets crossing at a wide angle (directions fixed in
  space, bent by a 45 km warp). Venusian, acidic, chthonian and rocky worlds.
* **Labyrinth** (Noctis Labyrinthus): a rise 0.8-2 km high cut by a maze of fault troughs on the boundaries of 30 km and 12 km cells, 6-10 and
  2.5-4 km wide, flat floors 2-5 and 0.9-2.4 km down between walls of three fault steps (the deeper of the two where they cross). The dry
  worlds that had water: thin air, desert, rust, dry ice, lichen, rocky.
* **Hollows** (Mercury, Blewett et al. 2011): clusters a few kilometres across of shallow flat-floored pits 60-500 m across and 15-40 m deep on
  half-kilometre cells, their outlines ragged, bright and bluish with a halo (`MAT_SALT`); the cluster's field marked at every scale (the
  scanner's 500 m scan reads it). Cratered, metal, bombarded, carbon, glass and rocky worlds.
* **Crater chains** (catenae: Enki, Gipul, Shoemaker-Levy 9): on 15% of the cells of a grid of a fifth of the radius (150-450 km), lines
  30-280 km long of six to seventeen craters of one size, their rims touching, bowls a fifth of their width deep (`chainOf`). Cratered, icy,
  rocky, bombarded, grooved ice, brine dwarf and two-faced worlds.
* **Penitentes** (the Andes; Europa's, Hobley et al. 2018; Pluto's bladed terrain, Moore et al. 2018): pointed blades in rows across the
  sun's path within 60 degrees of the equator (each a diamond longer along its row): on a living world's cold dry heights 2-5 m on rows 10-14 m
  apart, on the ices 4-10 m on rows 14-22 m apart (the 4 m ring's, as the ice spires), on a nitrogen world ridges 150-500 m high on rows 3-6 km
  apart; the field snow or ice. Not with R-304's spire fields.
* **Columnar basalt** (the Giant's Causeway, the Columbia River's coulees): a province of thick lava sheets, lobes on 2-5 km cells, two in three
  raised 15-40 m, each lobe's edge a colonnade; within the 4 m ring the edge runs along the columns' polygons (cells of 5-8 m: a column stands on
  the lobe its middle does) and the tops by the edge are a pavement of the columns' heads stepped by up to a metre; basalt in a band of 30 m
  by the edge (B-404: by the unfaded edge, in every ring that can hold it).
* **Mud volcanoes** (Gobustan; Mars' mounds): on low plains, cones 6-50 m high and 100-700 m across with a crater pool on a third of the cells
  of a 1.5 km grid (`mudConeOf`), two to four tongues of grey mud (`MAT_MUD`) running out one to three radii.
* **Inverted rivers** (Mars' Aeolis Dorsa): on dry plains, sinuous flat-topped ridges 8-40 m high and 150-500 m wide, the zero lines of a
  warped noise of 30 km and one of 10 (their branches where the nets meet), broken where the wind cut through, capped with rock.
* **Pingos** (the Mackenzie delta's): on cold flat lowlands, ice-cored hills 10-60 m high and 80-400 m across on a quarter of the cells of a
  1.2 km grid (`pingoOf`), the big ones split open at the top where the ice shows.
* **Maars** (the Eifel's): round craters 0.4-2 km across on 45% of the cells of a 4 km grid (`maarOf`), their floors 50-200 m under the plain
  within steep walls, a ring of tephra 8-28 m high and its apron. The drainage keeps a depression whose pit lies in a maar as a lake (it keeps
  two in five other small deep ones), so on a world it rains on about half of them are crater lakes, the others breached or under its 400 m
  lattice; on a dry world they are dry maars.

**The world's moisture and warmth.** Every ground but the living one passed the landforms a fixed moisture of 0.15 and the equilibrium
temperature alone: an acidic world's karst towers (moisture over 0.45, 8-16 C) never stood. A world it rains on now reads its moisture as the
living ground's field round its kind's mean (`KindDef::landMoist`: the hydrocarbon world 0.25, its methane rain rare and Titan's dune seas in
its dry tropics; ammonia 0.4; acidic 0.55; steam 0.6), a dry world's stays 0.15; every world reads the temperature its explorer's thermometer
reads (the surface's mean without the day: with air (T - 273) 0.75 + the kind's `tempOffset`, without (T - 273) 1.3 - 60, less its latitude and
its height, plus `tempAdd`). The hydrocarbon worlds' dune seas and yardangs and the acidic, steam and ammonia worlds' canyons, badlands and salt
flats keep to their drier parts; the thin-aired worlds' glaciers and patterned ground follow their cold (120 bodies of the stream's 44,632).

**The facts** (`bodyFacts`, `factList`; read from the orbit and the spin, no draw): SIDEWAYS SPIN (a tilt over 60 degrees), RETROGRADE SPIN,
RACING DAY (a day under half an hour of the game's clock: its days are 20 minutes to 4 hours, the fastest twelfth), LOCKED FACE, ECCENTRIC YEAR
(over 0.2: a moon's its planet's), STRONG FIELD and NO FIELD (R-402's classes), CAPTURED (a moon on a retrograde orbit), TWIN, RINGED.

## Q-02 (2026-10-22): the water, the sky, the life and the ground

**Sixteen traits in the new slots** (`TraitKind` `TK_WATER`, `TK_SKY`, `TK_LIFE`, `TK_GROUND`; rows `quality(...)` after the landforms, ids
45-60). The new slots' kinds in their hashed order now hold traits of five kinds, so a world takes one of each that fits it; the landforms'
share falls (Q-01's one landform a world where one fits gives way where the order puts another kind first). Each trait needs a process of its
own (`KS_TIDES` to `KS_METEORITES`, bits 36-51, given to the kinds in `buildKinds`'s Q-02 block) and three a fact of the world besides
(`traitFits`, in the draw): the great tides a forcing of 300 m (`Body::tideForcingM`, below), the great aurorae a strong field (R-402's class)
and an air, the barrier reefs a warm sea (22 C at the equator's shore). Those that move the ground or take one of the second people's palette
banks on the ground (`moves`: the tides, the hot springs, the pink lakes, the reefs, the fungal and petrified forests, the blooms, the strata,
the gypsum and the meteorites) are never on a world with a people nor the home world; the sky's and the milky seas are (Heilya II: noctilucent
clouds and milky seas). The fungal forests never come with R-304's dead forests (an exclusion one way: the new slots are drawn after the
first three, `traitsOk` allows it); the gypsum sands are a colour trait. The drainage's key reads the new slots that move the ground alone.

**The tides** (`galaxy/system.*`: `tidePulls`, `tideForcingM`, `tideRangeOf`, `tideOffsetM`). A world's forcing is the equilibrium tide
(m / M) R^4 / d^3 in metres that its moons and its sun raise, summed over the pulls that turn over it: none from a body its face keeps toward
it (a twin's, a locked world's sun, a facing moon's planet); a moon's are its planet's and its planet's sun. It is a derived fact
(`StarSystem::generate`'s last step, no draw). On the game's scale it is large (its moons at 5-40 radii, an AU of 22 million km: the scan
region's sea worlds' median 110 m), so the trait's coastal spring range is its compression: 5 m at 300 m of forcing (37% of the sea worlds),
4 m more each tenfold, 16 at most (`BodyGen::tideRangeM`). The sea's offset at a place and a time is each pull's P2 bulge less its mean over
the day at that latitude, weighted by its share: the range at the equator under a pull in the equator's plane, none at a pole, the spring and
neap tides and the high water's interval (twelve minutes to fifteen hours: the game's days are short) from the bodies' own motions.

**The tidal flats** (`tidalFlat`, the end of `sampleSurface`; `tideRangeAt`: the trait's range by the cos^2 of the latitude). The ground within
six ranges of the sea's level is pressed toward it (h' = S + H x (0.2 + 0.8 x^4), x the height over the level in units of H = 6 ranges): the
intertidal zone takes the ground of two ranges either side, a flat kilometres wide on a gentle coast, steeper above. Between low and high water
the flat is wet sand (albedo 0.3) under the sea's plane (its `water` the mean level, so the surface's tide moves the sheet over it), cut by a
dendrite of creeks a metre deep (a ridged noise, where the 16 m ring holds it); under the mean it is the sea's (`BIO_OCEAN`: the rivers end at
it). Rivers and lakes keep their own water.

**The waters' colours** (`qualityWater`; `SurfaceSample::waterTint`, `lifeTint`; `TerrainVertex::wtint`, `ltint`; `PlanetMap::wtint`, `ltint`):
the water kind's and the life kind's share of a water sample's second colour (`BodyGen::waterTintColour`, `lifeTintColour`). A pink lake's whole
water (a lake, a pool, a rift lake: `waterKind` 2-4; pink, rose or Natron's red by the world) with a white crust of salt on its shore; a milky
sea's patches of glowing bacteria (a field of a sixth of the radius, a few patches of a few hundred kilometres: Miller et al. 2005); an algal
bloom's (a red tide, a green, a brown by the world) on two scales, regions hundreds of kilometres across (a warped field: what the globe shows)
and in them filaments some 12 km apart along the field's zero lines where the eye resolves them, more over the shallows, on the seas and, but
where they are pink, the lakes; a barrier reef's lagoon (below). B-404's rule: unfaded fields, so every ring, the map and the globe agree.

**The barrier reefs** (`felisianSample`, the warm seas of a province of coasts, the climate at the sea's level 20-23 C and over): along the shelf's
edge where it falls past 18-40 m (a contour of the world's) the crest stands awash at -0.35 m, the fore-reef dropping seaward under it, passes
cut through it where a noise says; behind it the lagoon's floor lifted to 0.8-6 m, its water and the crest's shallows turquoise (`lifeTint`).

**The hot springs** (`landformsAt`, `springGrid`, `springOf`, `hotSpringsNear`): clusters of three to six springs within 0.7 km on a third of the
cells of a 3 km grid in a geothermal province, each a terraced mound 80-300 m across and 3-17 m high (four to eight terraces): the vent's pool
(`MAT_BRINE`, deep blue), white sinter round it (`MAT_SALT`), the terraces' rims white and their pools coloured by their mats (`MAT_MATS`, the
thermophiles' orange: bright near the vent, darker as the water cools; every other terrace white where the eye sees the terraces, the inner half
white from further), the runoff's mats streaking the ground round it.

**The fungal forests** (`materialPalette`: the forest family's colour cream, ochre, white or a lilac grey; `make`: the second family's another
pale) and **the petrified forests** (`landformsAt`'s province of the open dry ground; its density in `lifeTint` on the land for the surface's stone
trunks).

**The ground's** (`landformsAt`; `SurfaceSample::groundTint`, `TerrainVertex::gtint`): **rainbow strata** on the slopes of a province (the material
chain's slope over 0.1-0.22, B-404): bands 4-12 m thick on the material chain's height (waved by a 3 m noise), each the rock's own colour or the
accent's (a red, an orange or a green by the world: `groundTint` 1) at a brightness of its own, six times as thick where a ring cannot hold the
bands; **gypsum sands**, a province of the dry low country of white dunes (`MAT_SALT`: gypsum is an evaporite, the salts' white in every view)
100-250 m apart and 8-18 m high across the wind, gentle on the windward side and steep on the lee; **meteorite fields**, strewn fields on 22%
of the cells of a 70 km grid (`strewnGrid`, `strewnOf`: an ellipse 6-25 km long and 1.5-4 km wide along the way the body broke up), the stones'
density thicker downrange (`groundTint`), a few fresh pits 6-60 m across there with iron at their floors (`MAT_METAL`), marked at every scale
(the scanner's STREWN FIELD).

## Q-03 (2026-10-22): the sounds of the ground

**Three traits in the new slots** (`TK_SOUND`, rows `quality(...)` after the ground's, ids 61-63; heard, not drawn: `06-surface.md`,
`07-game-flow-and-controls.md`). SINGING DUNES (`KS_SINGING_SAND`): the deserts' and the rust worlds' dunes, the thin airs', Titan's, the dry
lowlands of the felisian, steppe, terminator and archean worlds; not a salt world's gypsum (silent, as White Sands is), nor a lichen world's
crusted dust, nor a scorched world's (its dunes were never dry: none of its sand stands in dunes). SINGING ICE (`KS_ICE_SONG`): the snowball,
the tundra, the frozen felisian, the terminator's and the eyeball's ice, the ammonia world's, the caps of the thin airs and the dry ice worlds,
the lichen worlds'. QUAKES (`KS_SEISMIC`): every kind with plates under an air but the tectonic world (its quakes are its kind's), and the moons a
giant's tides keep hot (the volcanic, the tiger-striped, the cryovolcanic). Bits 52-54, given in `buildKinds`'s Q-03 block; the dunes and the
ice need an air besides (`traitFits`). They move nothing (`moves` false): a world with a people and the home world may take them (Heilya II:
noctilucent clouds and singing dunes, its milky seas gone where its hashed order puts the sound's kind before the water's). The first build put
the quakes on every kind with plates or tides, the airless too: 9,834 of the scan region's 36,813 landable bodies (a cratered, a rocky or a
europan world's sound kind held them alone, as the airless rock's ground kind had held the meteorites in Q-02); the airless rock's and ice's
quakes come a few a year, not every few minutes, and they keep none. Over the scan region the sound kind holds 4,504 traits (1,312 singing
dunes, 787 singing ice, 2,405 quakes); the other kinds' fall where a world's order puts the sound first (the landforms 19,482 to 18,253, the
water 2,327 to 1,977, the sky 5,118 to 4,286, the life 1,412 to 1,228, the ground 2,913 to 2,308).

**The slip faces** (`SurfaceSample::slipFace`): how much a point is a sand dune's lee, the steep side downwind of its crest where the sand
slides, whatever the face's steepness: `duneField`'s `lee` (the profile's drop, 0.72-1.0 of its phase, for the transverse dunes and the
barchans; where the ground falls downwind for the star dunes) times the field's share where a ground lays its dunes (the felisian layer's dry
lowlands, the thin air's lowlands, the desert's basins, the hydrocarbon's low latitudes), and the dune seas' draa where they fall downwind
(`LandOut::slip`). An output alone: no height or material moves. The game's dunes stand from under 8 degrees on their lee (a rust world's
gentle ones) to over 30 (a thin air's), by their wavelength and height, and the drainage softens some (a hydrocarbon world's 26 to 13): a slope
alone could not tell a slip face from the windward side or a bank of dust.

## D-01 (2026-10-23): a drifter's worlds

The planet function needs nothing new for a drifter (`04-galaxy-and-systems.md`): its worlds are kinds of the table, made as any world of
their kind from their seeds, and the ground reads its warmth from `tempK` as ever, which for a drifter's body is its own heat
(`Body::heatK`; P-04's then-climate for a frozen felisian drifter, 265-300 K, as a wanderer's). `BodyGen::make` keeps a people off every
body of a drifter's system (`Body::drifter`, beside P-05's wanderer: the C-01 hash and a dead world's own are not read there, and the new
slots' `kept` rule follows): D-05 gives the drifters their peoples by a hash of their own. The drifters' own traits (frozen air, embers,
lights left on, lights in the water) and the dark ocean are D-04's.

## D-04 (2026-10-23): the ground in the dark

`BodyGen::dark` marks a body of a drifter's system; `sampleSurface` ends such a world's sample with `darkGround` after its kind's own
(nothing else reads it, so no other world moves; `thenWorld` clears it: a people's time had its sun).

* **The traits a body in the dark lacks** (`shapesOf`): a sun's processes (penitentes, hollows, halos, noctilucent clouds, singing ice,
  fog seas) and the life a sun feeds (reefs, fungi, blooms, pink lakes, milky seas); where its air froze, an air's and an open liquid's
  (storms, haze, airglow, aurorae, singing dunes, springs, mud, geysers); where its seas froze, the tides. Its own come first in the new
  slots (`darkTraitsOf`): FROZEN AIR where it is colder than its air's freezing point (`KindDef::airFreezeK`: 150 K, Titan's nitrogen
  under its haze 60, hydrogen never), EMBERS on every lava world and on three in ten of the rock drifters (rocky, cratered, metal,
  carbon, desert, frozen felisian; never one of ice), LIGHTS IN THE WATER on half the dark oceans; LIGHTS LEFT ON is D-05's.
* **Frozen seas** (`frozenSeas`: a sea, a wet world's lakes or an exotic sea colder than its kind's `seaFreezeK`: water 271 K, brine 250,
  ammonia water 176, methane 90, tar 300): a liquid sample is ice at its level (the floor raised to it), no sample keeps a water level, so
  `SurfaceSite::hasWater` is false and no sheet is drawn.
* **Embers** (`emberLakes`, a world with no lava of its own): fields of a young interior 80-250 km across on 45% of the cells of a 1,300 km
  grid (`emberGrid`, `emberFieldsOf`: the globe's spots of light), their plains young basalt; in a field lava lakes 160-600 m across on
  half the cells of a 3 km grid, fewer toward its edge: the lava (`MAT_LAVA`, glow 1) flat a few metres under the smooth ground inside a
  spatter rampart 4-12 m high, fresh black basalt to three and a half radii with glowing cracks in it, the snow kept off the warm ground.
* **The fallen air** (TR_FROZEN_AIR, every ground but the felisian layer's, whose frozen cover is it already): snow as deep on the open
  ground as its air was heavy (20 m a bar of the kind's pressure, 30 at most), the hollows filled from the smooth ground as far as it
  reaches, shed from slopes over 0.32-0.55, frost on the rest (the albedo raised; `materialPalette` tints every stone toward the frost's
  pale blue by a third, the snow nitrogen's white).
* **The dark ocean** (`GR_DARK_OCEAN`, `groundDarkOcean`): an abyssal plain 1.6 km down, swells of the hot spots half a kilometre down
  where its archipelagos stand (an unfaded field of a twelfth of the radius); shield volcanoes 8-21 km round on a 70 km grid, thicker and
  taller over the swells, breaking the surface as islands (a hundredth of the world, 0.6-1% on the six nearest home) with the skeleton's
  relief and landforms on them, spatter cones, young flows (O6-05's tongues), the waves' benches of black sand round their shores, pale
  crusts on the summits' fumarole fields; the sea at 0. Its lights in the water (`qualityWater`): shoals some hundreds of kilometres across
  and the islands' flanks under 500 m, a quarter of their water lit in patches of a hundred metres or two; from afar the shoals' cores'
  sheen.
