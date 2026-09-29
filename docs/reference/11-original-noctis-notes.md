# Notes on the original Noctis IV

Collected from the released source (dgcole/noctis-iv-lr port of Ghignola's code) and the manuals, to guide fidelity work. Marked with what this clone does.

## Rendering

* 320x200, 256 colours in four banks of 64 (`psmooth_64` averages the low six bits of a 2x2 block, keeps the high two). **Replicated** (`Framebuffer::mush`).
* Polygons filled by `poly3d`/`polymap` (textured with 256x256 noise textures, "flares" modes for translucency/glow). Terrain drawn per 32.768 m quadrant, culled beyond 64 quadrants diagonally; nearest quadrant drawn once; distant quadrants get +depth/2 shade (fog); desert reverses culling for finer distant grain. **Partially**: flat quads with grain, fog in the ramp, three LOD rings instead of a single 200x200 map.
* Sky as a QuickTime-VR style cylindrical panorama (`background`), stars from the same sector hash as in space, sun via `white_sun` (disc + dispersion), `lens_flares_for` halos, `far_pixel_at` for other bodies. **Partially**: per-pixel sky, real bodies in the sky, no lens flares yet (M1-03).
* Water: mirrored terrain drawn with `halfscan`, incoming and outgoing wave rings, sparkles toward the sun. **Not yet** (M1-04).
* Planet globes via precomputed offset maps (`globes.map`) and 360x180 surface maps; day-night terminator by shifting 130 degrees of the map by two shades. **Replaced** by ray-traced globes with real lighting.

## Generation

* Stars: one per 100000-unit sector, position from multiplicative hashes of the sector coordinates, rarity increasing with distance from the galactic core (bits masked out). Class from `brtl_random(12)` seeded by coordinates. **Replicated in spirit** with a density function (disk, bulge, arms) and weighted classes.
* Twelve star classes S00-S11: yellow, blue giant, white dwarf, red giant, orange giant, brown dwarf, grey giant (lithium), blue dwarf, multiple system, infant star (gas clouds), runaway star, pulsar. **Six chosen**: yellow, orange, blue giant, red giant, white dwarf, pulsar. Candidates to add: multiple systems with companion stars (planet type 10 in the original), brown dwarf/substellar objects (type 9), infant stars with proto-planets.
* `prepare_nearstar`: planets `random(class_planets+1)`; orbit radii accumulate (Kepler-like) with 22% growth beyond the eighth orbit; type constraints per class and per orbit index (hot inner, cold outer); moons per type (`planet_possiblemoons`), moon type rules (habitable moons only around gas giants, frozen moons far out), rings for gas giants and rarely others. **Replicated** with temperature zones instead of orbit indices.
* Planet types 0-10: molten (0), cratered (1), venusian (2), felisian (3), rocky (4), thin atmosphere (5), gas giant (6), icy (7), quartz (8), substellar (9), companion star (10). **Nine of them.**
* `surface()` builds a 360x180 albedo map by processing a random field with smoothing (`ssmooth`), craters (`crater_juice`), bands, waves, fractures, volcanoes, storms, cyclones; atmosphere overlay at half resolution; rotation period `10*(1..50)+10*(0..24)+(0..249)+41` seconds per degree. **Replaced** by the noise-based planet function; rotation periods are minutes to hours instead of hours to days.
* `build_surface()` (200x200 heights, 32.768 m): per type: cratered `rockyground` + `std_crater`; venusian `round_hill` domes; felisian scenarios OCEAN/PLAINS/DESERT/ICY chosen from the landing albedo and latitude (ice above 75 deg), islands, waves; rocky boulders as steep `round_hill`s; thin atmosphere craters and bright-cloud regions; icy cracks (`srf_darkline`); quartz domes; objects (rocks, vegetation, trees) distributed on flatter cells; ruins on specific historical systems. **Replicated in spirit**, with the climate derived from the map rather than the albedo.
* `create_sky()`: four colour ramps (ground, sky, horizon/sea, vegetation) derived from the star colour, the landing albedo, the planet type and the scenario; night variants; temperature and pressure estimates. **Replicated** (`lookFor` + `setupPalette`).
* Sun position: `latitude = |landing_lat - 60| * 1.5`, `exposure` from the crepuscular zone (distance to the terminators), sun coordinates `(-d cos b, -d sin b sin a, d sin b cos a)`. **Replaced** by exact geometry (spin axis, tilt, rotation, orbit).

## Ship and interface

* Walkable Stardrifter cabin (5x5x1 m) with the front screen (Flight control / Onboard devices / Preferences menus), GOES.net console on the left wall (SL, DL, PAR, ST, WHERE, CAST, CAT, INBOX/OUTBOX, CLEAN), three screens on the right for the landing map, fuel orb, capsule net, roof deck, depolarise (transparent walls), internal light. **Not yet** (M2).
* Remote target by crosshair (double right click) or by parsis coordinates; Vimana flight with `pfade` streaks; local target and fine approach; tracking modes fixed-point chase, far chase, synchronous orbit, high-speed orbit, near chase. **Partially** (crosshair, Vimana, approach, orbit/chase).
* Surface: landing sector chosen on a map (arrows, Enter), capsule with a light beam, walking limited to ~1.5 km from the capsule (3 km in later versions), jetpack (Space/Tab, speeds 0-9), stand (S), jump (J), vision modes (P), supervision (O), anti-fog (F), radiation visor (Page Up/Down), snapshots (M, N panoramic, F3 movie), sector map (F10), portable GOES (F5), planet finder locator line. **Partially**: no walking limit, no jetpack/vision modes/photo modes yet (M4, M6).
* Time: EPOC:triads display, game clock = real clock since 1984 (planets moved in real time). **Different**: own clock with time warp; EPOC formatting kept.
* Lithium fuel, help requests, lithium scoping at grey giants. **Dropped** on purpose (no goal, no fail state).
* The GUIDE: shared catalogue of named stars and planets with notes, updated by the community. **Not yet** (M3).
