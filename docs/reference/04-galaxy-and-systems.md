# Galaxy and planetary systems

## Star field (`galaxy/starfield.*`)

One potential star per sector `(sx, sy, sz)`:

```
seed    = hash3i(sx, sy, sz, 0x5EED5EED)
exists  = unitFromHash(seed) < galaxyDensity(sx, sy, sz)
pos     = (sector + 0.1 + 0.8 * u) * SECTOR_KM per axis
```

`galaxyDensity` (sector units): `r = sqrt(sx^2 + sz^2)`, scale height `h = 10 + 45 exp(-r/130)`, `disk = 0.85 exp(-r/230) exp(-|sy|/h)`, `bulge = 0.9 exp(-(r^2 + 4 sy^2) / 70^2)`, two spiral arms `arm = 0.5 + 0.5 cos(2 ang - r/45)`, density `= min(0.97, disk (0.55 + 0.6 arm) + bulge)`. The new-game search looks at sectors x 176..195, z 36..55 (about r = 190, density ~0.2).

Class pick: weights = rarity {S00 30, S01 28, S02 8, S03 12, S04 14, S05 8}, modulated by `bulge = exp(-r/90)`: red giants x(1 + 1.5 bulge), white dwarfs x(1 + bulge), blue giants x(1 + 0.8 (1 - bulge)). Radius = class radius x (1 +- radiusVar); luminosity = class L x rv^2 x [0.85, 1.15]; massFactor x [0.8, 1.2]; colour tint +-0.06 between r and b; pulsars pulse at 0.6-3.6 Hz. Names come from `generateName(seed)` (syllable generator, retried until at most 14 characters, 8% two-word names).

### Star classes (`STAR_CLASSES`)

| Code | Name | Colour | Radius km (var) | L | mass | max planets | rarity | first orbit | min first orbit km |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| S00 | Yellow star | (1.00,0.93,0.72) | 7.0e5 (25%) | 1.0 | 1.0 | 10 | 30 | 20 R | 8e6 |
| S01 | Orange dwarf | (1.00,0.72,0.42) | 4.6e5 (25%) | 0.35 | 0.7 | 8 | 28 | 16 R | 5e6 |
| S02 | Blue giant | (0.66,0.78,1.00) | 4.5e6 (35%) | 60 | 12 | 16 | 8 | 14 R | 5e7 |
| S03 | Red giant | (1.00,0.45,0.25) | 3.5e7 (40%) | 22 | 1.4 | 6 | 12 | 6 R | 2e8 |
| S04 | White dwarf | (0.86,0.90,1.00) | 9.0e3 (30%) | 0.02 | 0.8 | 4 | 14 | 1 R | 6e6 |
| S05 | Pulsar | (0.62,0.62,1.00) | 2.0e4 (20%) | 0.05 | 1.6 | 3 | 8 | 1 R | 1e7 |

Class effects elsewhere: sun rendering (`SpaceRenderer::drawSun`: granulation and spots for S00/S01, glare for S02, mottled convection for S03, diffraction rays for S04, pulsing + rotating beams for S05), planet type weights (below), light colour on surfaces, radiation warnings in the space HUD (pulsar `4e7/d`, white dwarf `1.2e7/d`, blue giant `1.5e8/d`, warn above 0.4, hazard above 1.0).

## System generation (`StarSystem::generate`)

Seeded with `star.seed ^ 0x51A7E11`. Steps:

1. Planet count `np = (irange(max+1) + irange(max+1) + 1) / 2`; pulsars have a 30% chance of none.
2. First key radius `= max(R * firstOrbitMult, minFirstOrbit) * [0.8, 1.3]`.
3. For each planet: orbit `= key * (1 +- 0.08)`, `Teq` from the orbit, type from the zone table, radius from the type range by `sizeRand`, mass `= (R/6371)^3 * density` (gas giant 0.25, icy 0.55, else 1), gravity `= 9.8 mass / (R/6371)^2`, period, phase, inclination `+-4 deg * [1,2]`, node, rotation (first planet 50% tidally locked, else `1200 + 12000 u^2` s; 8% retrograde), tilt (60%: 0-12 deg, 33%: 12-35, 7%: 35-95), rings (gas giants 60%, others 7%; inner 1.35-1.95 R, outer inner + 0.5-2.1 R), colour by type (`typeColor`), name `<Star> <roman>`.
4. Next key `= orbit * [1.45, 1.95]` (x1.25 after a gas giant) for the first eight orbits, then `orbit * [1.18, 1.30]` (the original's slower growth beyond the eighth orbit).
5. Moons per planet: `nm = avg of two draws in [0, maxMoons]`; gas giants `2 + irange(maxMoons - 1)`; the first planet gets at most one unless it is a gas giant; 60 bodies total cap. Moon key radius `= Rp * [3.5, 6.5]` (M5-09: 2.8-4.2; B-314, generation 7: `max(4.5-6, 1.35 x ringOuter)` radii), growth x[1.6, 2.4] for the first two then x[1.25, 1.55]. Moon radius `min(maxR, type range by sizeRand^2)` with `maxR = 6500` for gas giant parents else `0.42 Rp`, at least 600-1000 km. Period with the parent mass; inclination +-6 deg; 70% tidally locked else 900-7900 s; tilt 0-8 deg; name `<planet>-a`, `-b`, ...
6. Spin frames for every body.

Important: the planet loop copies the parent (`Body p = bodies[n]`) because `push_back` invalidates references.

### Planet type selection (`pickPlanetType`)

Zones by equilibrium temperature (weights):

| Zone | Weights |
| --- | --- |
| > 600 K | molten 40, cratered 35, venusian 18, rocky 7 |
| 330-600 K | venusian 28, cratered 24, rocky 20, thin 10, molten 12, quartz 6 |
| 215-330 K | felisian 34, thin 20, rocky 14, cratered 12, quartz 10, venusian 10 |
| 120-215 K | thin 18, icy 22, gas giant 28, rocky 14, cratered 13, quartz 5 |
| < 120 K | icy 42, gas giant 32, cratered 20, rocky 6 |

Modifiers: white dwarf / pulsar: no felisian, venusian, quartz; thin x0.3; cratered +20; icy +15; pulsar also gas giant x0.3, molten x0.3. Red giant: felisian x0.25, molten x1.5. Blue giant: felisian x0.5. Moons: no gas giants; venusian x0.2; unless the parent is a gas giant, felisian x0.15 and quartz x0.3; cratered x1.6, icy x1.4. Small size roll (< 0.3): gas giant x0.2.

### Planet types (`PLANET_TYPES`)

| Type | Radius km | Landable | Atmosphere | Max moons | Albedo |
| --- | --- | --- | --- | --- | --- |
| Molten | 2000-4200 | yes | no | 1 | 0.25 |
| Cratered | 900-3200 | yes | no | 1 | 0.20 |
| Venusian | 4800-7200 | yes | yes | 2 | 0.75 |
| Felisian | 4800-7600 | yes | yes | 3 | 0.35 |
| Rocky | 2800-6000 | yes | no | 2 | 0.18 |
| Thin atmosphere | 2400-4600 | yes | yes | 2 | 0.30 |
| Gas giant | 24000-72000 | no | yes | 12 | 0.55 |
| Icy | 900-2600 | yes | no | 2 | 0.70 |
| Quartz | 3800-6200 | yes | yes | 3 | 0.80 |
| Ocean / Metal / Volcanic / Carbon (M9-11) | 5000-7800 / 1200-3200 / 1200-2600 / 2500-5000 | yes | yes / no / no / no | 2 / 1 / 0 / 1 | 0.30 / 0.12 / 0.50 / 0.06 |
| Substellar (M5-02) | 60000-95000 | no | yes | 8 | 0.30 |
| Comet (M5-07; landable since O4, R-303) | 2-12 | yes | no | 0 | 0.06 |

Colours (`typeColor`) carry per-body variation; thin atmosphere and gas giants pick one of four families (red/green/purple/grey; cream/blue/brown/pale).

## Observed statistics (test `stars`, 40x40 sectors at y = 0)

About 20% of sectors hold a star near the start region. Bodies are dominated by cold zones (icy, cratered) because most orbits are far out; felisian worlds are about 3% of bodies, roughly one per three systems. Tune the zone table if that feels sparse.

## Sky statistics (survey, 2026-09-26)

Measured with a throwaway program over 2,646 systems near the start region (28,484 landable bodies: 36% planets, 64% moons, 39% moons of gas giants), random sites and times. A body with angular radius above 1 deg is above the horizon at 37% of sites (41% on moons, 31% on planets), above 3 deg at 28%, above 10 deg at 10%; another planet is above the horizon at 77% of sites but only as a sub-pixel point (largest ever 1.2 deg). Discs are the parent planet (73% of cases) or the body's own moon (26%); sister moons under 1%. Moon counts per planet in that region: 0: 35%, 1: 41%, 2: 9%, 3-6: 5%, 7+: 7% (gas giants). Request R-001 and roadmap M5-09 build on these numbers; `vesperis_test survey` (M9-01) will recompute them.

## Generation version 2 (M9, 2026-09-26)

* **Regions** (`galaxyRegion`, `REGION_NAMES`): core (r < 35, |y| < 12), bulge (bulge term dominant, r < 110), halo (|y| > 2.5 scale heights), globular clusters (hashed 6-sector blocks, density x2.5), star-forming regions (hashed 14-sector blocks on the arms), spiral arms (arm factor > 0.7), disk. Class weights per region: the core favours pulsars and giants, the bulge and halo remnants and red giants, clusters old stars, star-forming regions blue giants. Names use a style per region (`generateName(seed, style)`: hard consonants in the core, bulge and halo; flowing syllables in arms and nebulae). The space HUD shows the region after the sector.
* **Systems**: orbit spacing `1.32-1.77x` (tighter); zone weights raised for felisian (50 in 205-345 K) and ocean worlds; four new types with their own weights (metal near the star, carbon far out and around remnants, ocean in the temperate band, volcanic only as inner moons of gas giants, which get +60 K of tidal heating per rank); moons draw from a modified table (fewer icy, more rocky and thin-air). Asteroid belts (`StarSystem::belts`) fill wide gaps (55% when the ratio exceeds 1.65, else 12%) and sometimes sit beyond the last planet; they are listed in the analyzer and drawn as 260 hashed rocks. **O3 (R-302):** a belt is a destination (`07-game-flow-and-controls.md`) and its rocks are a pure function of the system: the belt is cut into rings `BELT_CELL_KM` (400 km) wide in radius, each ring turning as one at the Kepler rate of its middle radius (`Belt::rateAt(r) = 2 pi / period x (inner / r)^1.5`, the sparkle band's shear: inner rings outrun outer ones, a ring keeps its formation), into arc cells of 400 km round the star (`beltArcCell`: the cell of a world angle at a given ring, taken modulo the ring's cell count) and height cells; a cell hashes 1-3 rocks (`BELT_ROCKS_PER_CELL`; fewer within 12% of the edges; none above 1.5% of the radius in height), each with a position, a radius `0.25 + 12 u^4` km (one in two thousand +25 km), a seed, a tumble axis and a period of 30-150 min (`BeltRock`, `StarSystem::beltRockAt(k, ir, ia, iy, m, t)`); `beltCellOf` gives a position's cells and `forBeltRocksNear` visits every rock within a few cells of a position ring by ring. The first version hashed cells in one co-rotating frame and, at t = 3.6e6 s, the rings had sheared by hundreds of cells: every rock but one was lost from the search around the ship. Every other moon locks its period to the previous one (2:1 or 3:2); 18% of gas giants keep a captured small moon far out on a tilted retrograde orbit (negative orbital period); moons inside a ring carve a gap in the ring profile.
* **Descriptions** (`describeBody`, `describeStar`, `galaxy/describe.cpp`): one sentence from size, type parameters, gravity, temperature, rings, moons and rotation, shown on the data sheet.
* `GEN_VERSION` (2) is written into saves (`gen` line) and guides; loading an older save shows a one-time warning. Star positions are integer hashes and did not move.

Near the start (`vesperis_test survey 1500`): felisian 9% of bodies and in 54% of systems; cratered 22%, icy 20%, rocky 16%, thin atmosphere 14%, gas giant 5%, quartz 4%, carbon 3%, venusian 2%, ocean 2%, volcanic 1%, metal 1%, molten 0.5%.

## Generation version 5 (O0-01, 2026-09-27)

`GEN_VERSION` is 5: solid planets wear rings with 12% instead of 7% (`rng.chance(0.12)` on the same draw, so nothing else in a system moves; the regression's generation hashes did not change). Near the start 2.4% of landable bodies were ringed planets before (7% of solid planets) and 22% are moons of a ringed world. The ring profile is `StarSystem::ringProfileOf(bi)` (256 samples of fbm bands with hashed gaps and the shepherd moons' gaps), shared by the globe, the sky arc and the ring shadow on the ground.

## Generation version 3 (M5, 2026-09-26)

`GEN_VERSION` is 3: systems gained bodies and orbits changed shape, so saves and guides from version 2 still load but their systems differ; `tests/regress_baseline.txt` was re-blessed.

* **Kepler orbits (M5-03).** `Body::ecc` and `argPeri`; `orbitRadiusKm` is the semi-major axis and `orbitPhase0` the mean anomaly at t = 0. `StarSystem::trueAnomaly` solves `E - e sin E = M` by Newton (12 steps, started at `M + e sin M`); `bodyPos` places the body at `a (1 - e cos E)` along the true anomaly plus the argument of periapsis. Eccentricities come from a separate stream (`Rng(seed ^ 0x0ECC)`): planets 60% below 0.06, 32% 0.06-0.15, 8% 0.15-0.3; moons below 0.04; captured moons 0.1-0.4; companions 0-0.1 (close) or 0.05-0.45 (wide); comets 0.6-0.93.
* **Companion stars (M5-01).** After the moons, `Rng(seed ^ 0xB1A2)` gives each class a chance of a companion (yellow 18%, orange 20%, blue 28%, red giant 12%, white dwarf 24%, pulsar 15%): a body of type `PT_COMPANION` with `starClass`, a radius from the class table, `luminosity = class L x (R / R_class)^2 x 0.7-1.3`, its colour as `color`. 35% are close binaries at 3-6 summed radii when that stays inside 40% of the first orbit (the planets then circle both); the rest orbit at 2.5-7.5 times the widest planetary orbit, inclined up to 25 deg, and carry 0-3 worlds of their own (`parent` = the companion, named `<star> B I..`, lit by both stars through `T^4` addition, no moons). `StarSystem::companion` indexes it; `classString()` reads `S00+S01 MULTIPLE`; `companionStar()` builds a `Star` for the sun renderer; `lightAt(pos, t)` returns the direction and perceptual factor (`starLightFactor`, the old `SpaceRenderer::lightFactor`) of whichever sun is brighter at a point. The primary stays at the system origin (no barycentric motion, KI-019).
* **Substellar objects (M5-02).** `PT_SUBSTELLAR` (60,000-95,000 km, not landable) with weight 4 in the cold band (below 205 K), never as a moon; `luminosity` 0.0008-0.005 warms its moons (`T^4` addition) and sets `BodyGen::selfGlow`. Moons use the gas giant table, 2-8 of them, tidal heating and captured moons as for giants.
* **Nebula patches (N5-01).** `nebulaPatches(obsSectors, out, max)` looks at the 3x3 nebula cells (14 sectors) around the observer that pass `nebulaAt` and the arm test of `galaxyRegion`; each cell hashes 3-6 patches (`hash2i(cell, 0x4EBA)`) at points inside it (y within 3 sectors of the plane), 2-5 sectors across, giving a direction, an angular radius (`atan(size / dist)`, 2-30 deg), a tone (blue 45%, red 35%, white) and a strength (`1.3 - dist / 22`, 0.12..1). `nebulaGlow(patches, dir)` returns the strongest patch's `smoothstep(r, 0.2 r, angle) x wisps (gnoise3) x strength`. Generation version 4 (N5-03) only changes gas giant maps' veg channel; star positions and systems are unchanged.
* **Comets (M5-07).** `Rng(seed ^ 0xC0E7)`: 45% of systems hold one or two `PT_COMET` bodies (2-12 km) with `a` = 0.35-1.55 times the widest orbit (periapsis kept beyond four star radii), inclination up to 40 deg, names `<star> comet I`. They are listed, targetable, approachable and, since O4 (R-303), landable (`06-surface.md`, small bodies). N0-01: `StarSystem::bodyVel(i, t)` (km/s, central difference over `1e-4` of the period, at least 1 s) and `meanAnomaly(i, t)` (0..2 pi, 0 = periapsis, below pi = receding) feed the dust tail, the HUD speed and the `PERIAPSIS IN` / `APOAPSIS IN` readouts. Rates are physical: the sample comet of `space_comet.png` does 740 km/s at periapsis, which is a pixel every 25 s at x1 from the first planet and 18.6 px per 500 s at x100.
* **Double planets and moon tuning (M5-09).** 7% of rocky, felisian, venusian, quartz, ocean and thin-atmosphere planets get a twin (0.55-0.95 radii at 5-9 radii, both `doublePlanet`, both locked to each other, the planet's day set to the twin's period). The first moon of a solid planet sits at 2.8-4.2 radii (3.5-6.5 before), may reach half the planet's radius (0.42 before) and is big (size roll 0.5-1) half the time; giants' first two moon spacings are 1.35-1.85 (1.6-2.4 before). **B-314 (GEN_VERSION 7, 2026-09-28):** the first moon of any planet orbits at 4.5-6 radii, and at 1.35 times the ring's outer radius at least, on the same random draw: from 2.8 radii the parent's disc was 42 degrees across and a moon inside the rings (they reach 4 radii) saw them fill the sky; from 4.5 radii the disc is 26 degrees. Moon orbits and periods change everywhere; surfaces and maps do not.
* **Locked faces (M5-08).** After the spin frames, a planet whose day equals its year (within 2%, eccentricity below 0.05, not a twin's parent) gets `locked` and `lockedDir`, the body-frame direction of its star at t = 0; the planet function reads it (`05-planet-function.md`).

Survey after M5 (4,000 bodies): multiple systems 19%, substellar objects 10, comets 216, double planets 50, locked planets 63; sky discs at random sites: over 1 deg 39%, over 3 deg 33%, over 10 deg 18%, a sister moon 8% of the discs, two suns up 5% of sites.
