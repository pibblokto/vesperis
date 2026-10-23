# Milestones for 1.1.0 and beyond

The ideas in this directory, cut into milestones with ids to reference in requests, bug
reports and `PROGRESS.md` ("proceed with C-02"). Each item names the section of its
document. Sizes: S (an afternoon), M (a day or two), L (a week), XL (a milestone of its own).
Order is a suggestion.

## Status

**Released as 1.1.0 (2026-10-03), generation 11:** G-01 to G-04, S-01 to S-06, C-01 to C-10 and
C-12 to C-14 (the C series in its own order on the user's call of 2026-10-02: C-01, C-02,
C-03, C-06, C-04, C-05, C-07, C-08, C-10, C-09, C-12, C-13, C-14), with R-402, R-403 and
B-403 to B-406 (`docs/iteration-3/plans/PROGRESS.md`). Each table below carries a `Status`
column: `1.1.0`, `1.2.0`, `1.3.0` or `1.3.1` shipped in that release, `1.4.0` is for the next one.

**Released as 1.2.0 (2026-10-07), generation 12:** the iteration `ITERATION-1.2.0.md`: W-01, W-06, W-03,
W-04, X-01 to X-04, with R-404 to R-408 and B-407 to B-410.

**Released as 1.3.0 (2026-10-11), generation 13:** the iteration `ITERATION-1.3.0.md`, planned 2026-10-08 as one release with
everything in it, the far, the faint and the peoples, on the user's three requests (R-409 a chart of the whole galaxy, R-410 a radar turned up to hear
farther, R-411 the civilisation expansion of `IDEAS-civilisation-expansion.md`), in the order the user left to the plan:
G-05, G-06, G-07, H-01, A-01, C-15, C-16, A-02, A-03, A-04, T-01, H-02, T-02, T-03, T-04, H-03, H-04, H-05, H-06, C-11;
generation 13 from A-01. C-17 (the instruments' maker) was added on 2026-10-10 after C-11, on the user's request, the twenty-first; C-18
(the drone bay) on 2026-10-11, the twenty-second; with R-412 to R-417 and B-411 to B-422. The rows not done move to `1.4.0`.

**Released as 1.3.1 (2026-10-15), generation 13 unchanged:** the performance plan `PLAN-performance.md`, F-01 to F-11 with
B-423 and B-424: the frame about twice as fast with the same pictures, the stalls a player felt cut down.

**Released as 1.3.2 (2026-10-19), generation 15:** the user's patch, their items one at a time on the branch `release/v1.3.2`:
R-418 (the controller), B-425 (the companion stars; generation 14), B-426 (a capital's records), R-419 (auto-explore), B-427 (a
Wolf-Rayet star's giants' tail), B-428 (the moons spread; generation 15) and R-420 (auto-explore's tape).

**Planned for 1.4.0 (2026-10-21), generation 16:** the iteration `ITERATION-1.4.0.md` on the branch `release/v1.4.0`, on the
user's request of 2026-10-21 (R-421 thirty to fifty new kinds of world, R-422 more modifiers, R-423 rogue planets found by
radio), the design in `IDEAS-worlds-and-wanderers.md`: sixteen milestones, P-00 to P-07 (the P series re-cut: the old list's
twelve kinds are in its families), Q-01 to Q-03 and D-01 to D-05, in that order; generation 16 from P-01.

**Left for later:** the V series (the surface picture, parked), S-07, W-05, the L and K series (C-11, skipped under KI-347 in 1.1.0, done in 1.3.0 on 2026-10-10: the iteration's twenty are done, and C-17 added after them); W-02 comes into 1.4.0 as Q-02's great tides.
Two additions of the user's are in `IDEAS-ruins-shards-signals.md` section 5 (ruins with a
lot more variety and interiors; not every desert is post-apocalyptic).

## G: the galaxy (`PLAN-galaxy-scale.md`) — one generation bump with S

| Id | Item | Size | Status |
| --- | --- | --- | --- |
| G-01 | New density constants: ~200 billion stars, Milky Way proportions, local density kept | M | 1.1.0 |
| G-02 | Regions, globular clusters, start sector and home star in the new disc | S | 1.1.0 |
| G-03 | Galaxy background integration at the new scale (two-scale ray integration) | M | 1.1.0 |
| G-04 | Scene finders and pinned sites re-found, `regress bless`, flow green | S | 1.1.0 |
| G-05 | The galaxy chart: the star map's second page, the whole disc by its stars, the ship, home, the route (gone with R-414), the locked signals | M | 1.3.0, done 2026-10-08 |
| G-06 | The next jump chosen on the chart: a cursor and what lies there, the jump plotted and flown, a look before the leap | M | 1.3.0, done 2026-10-08 |
| G-07 | The crossing: the galaxy turning round the ship in a long flight (KI-364) | S | 1.3.0, done 2026-10-08 |

G-05 to G-07 were added on 2026-10-08 for 1.3.0 (R-409): the galaxy as the explorer sees and crosses it, after the bump; no
generation change.

## S: star classes (`PLAN-star-classes.md`) — same bump as G

| Id | Item | Size | Status |
| --- | --- | --- | --- |
| S-01 | Plain rows: red dwarf (flares, tidal locking), blue-white, orange giant, carbon star; region weights | M | 1.1.0 |
| S-02 | UI seams: class filter beyond six bits, codes S06+, "classes seen" from the table, descriptions | S | 1.1.0 |
| S-03 | Neutron star (debris system, glassed worlds) | M | 1.1.0 |
| S-04 | Protostar (nebula sky, dust and belts, accretion disc) | M | 1.1.0 |
| S-05 | Wolf-Rayet star (shell ring in the sky, stripping wind, radiation) | M | 1.1.0 |
| S-06 | Black hole: lensing, accretion ring, drawn-out companion, survivor rocks | XL | 1.1.0 |
| S-07 | Later rows when wanted: yellow supergiant, magnetar, T Tauri, white dwarf pair | S each | 1.4.0 or later |

## C: civilisations (`IDEAS-ruins-shards-signals.md`)

| Id | Item | Size | Status |
| --- | --- | --- | --- |
| C-01 | The "had a civilisation" trait on desert and felisian worlds; ruin density and a second ruin style; post-apocalyptic deserts keep dry seabeds and old rivers | M | 1.1.0 |
| C-02 | Story grammar prototype as a harness mode printing fifty shards per world; vocabulary from the world's data; recurring proper nouns; tone by world type | L | 1.1.0 |
| C-03 | Shards as objects in ruins: placement, finding, the log and the guide statistics | M | 1.1.0 |
| C-04 | Music: per-world tradition (scale, cycle, timbres) and generated pieces; name a piece | L | 1.1.0 |
| C-05 | Language: phoneme inventory, prosody, voice synthesis; alien speech aligned with the decoded text | L | 1.1.0 |
| C-06 | Decoding on the ship: the cabin screen, the wait, the decoded text; a language you learn (30% first, re-decoding as shards accumulate) | M | 1.1.0 |
| C-07 | The signal radar: galaxy-hashed signals, the sweep, lock, distance and age, rare and far; non-civilisation signals (pulsar, comet, magnetosphere) | L | 1.1.0 |
| C-08 | Ruins that read the world: harbours, terraces, cisterns and walls from the terrain; weathering by age | L | 1.1.0 |
| C-09 | Roads that still go somewhere: faint lines in the terrain, on the landing zoom, between ruins | M | 1.1.0 |
| C-10 | Star charts as shards: the culture's sky overlaid on yours, pointing at other dead worlds | M | 1.1.0 |
| C-11 | Instruments as artefacts, playable from the cabin | S | 1.3.0, done 2026-10-10 (KI-347; the ship's player and the instruments of the ages, last of the iteration) |
| C-12 | Graves and names; a culture's calendar and the timeline; the last recording per world | M | 1.1.0 |
| C-13 | Cultures per world: none, one or (rarely) two | S | 1.1.0 |
| C-14 | Lending: decoded shards travel with the guide export, credited in cyan | S | 1.1.0 |
| C-15 | Deep listening: the radar's gain, a power per transmitter, the far scan, far signals faint and slow to lock | M | 1.3.0, done 2026-10-08 |
| C-16 | What the gain lets in: the floor, the galaxy's own noise, signals mixed, far voices worn, voices in the static | L | 1.3.0, done 2026-10-09 |
| C-17 | The instruments' maker: each people's instruments built from parts by its own way of making (the user's request after C-11) | L | 1.3.0, done 2026-10-10 |
| C-18 | The drone bay: three drones with hands bring a people's world's records and instruments to the ship; the instruments' case by world, a world of two peoples a folder each people (the user's request after R-412) | L | 1.3.0, done 2026-10-11 |

Order: C-01, C-02 (judge the text before any audio), C-03, C-06, C-04, C-05, C-07, then
C-08 and C-10 as the pair that connects the galaxy, the rest as wanted. C-15 and C-16 were added on 2026-10-08 for 1.3.0
(R-410): the radar turned up to hear farther, through the noise; no generation change.

## A: the ages and what they built (`IDEAS-civilisation-expansion.md`) — generation 13 with T and H

| Id | Item | Size | Status |
| --- | --- | --- | --- |
| A-01 | The ages: each people's age at its collapse (prehistoric to far future), its profile, who transmits | M | 1.3.0, done 2026-10-08 |
| A-02 | The forms the renderer needs: new shapes and their broken states, colliders, LOD to a skyline, a people's colours in banks | XL | 1.3.0, done 2026-10-09 |
| A-03 | What each age builds: buildings, materials, heights and density of six ages; decay by material; older layers | L | 1.3.0, done 2026-10-09 |
| A-04 | The architecture genome: thirty-five deep variables and the age's own; the architecture sheet; distinctness measured | L | 1.3.0, done 2026-10-09 |

## T: the towns and the works (`IDEAS-civilisation-expansion.md`)

| Id | Item | Size | Status |
| --- | --- | --- | --- |
| T-01 | A people's map of its world: a plan of capital, cities, towns and villages by the age, networks, seen from orbit | XL | 1.3.0, done 2026-10-09 |
| T-02 | Towns by age: plans, districts, density and height, edges, the site's reading by age | L | 1.3.0, done 2026-10-09 |
| T-03 | The works: everything else people make, each placed by a rule of the terrain and the history | L | 1.3.0, done 2026-10-09 |
| T-04 | Where the records lie: the shards' places and objects by age, the count by age and size | M | 1.3.0, done 2026-10-09 |

## H: the histories and the records (`IDEAS-civilisation-expansion.md`)

| Id | Item | Size | Status |
| --- | --- | --- | --- |
| H-01 | Research: grammars and histories for generated records; a written choice and a prototype to read | M | 1.3.0, done 2026-10-08 |
| H-02 | The history: a simulated history per people, eras, figures, institutions, events, the collapse by its cause | L | 1.3.0, done 2026-10-09 |
| H-03 | The grammar engine: the chosen formalism, the realiser, the authoring format, the checks; C-02's texts rebuilt on it | L | 1.3.0, done 2026-10-09 |
| H-04 | The records: genres by age, longer texts, dozens of inputs, the collapse from many sides; reworked: each people its own (tongues, culture, the everyday) | XL | 1.3.0, done 2026-10-10 |
| H-05 | More shards in three categories styled by the age: texts, music, star charts; the radar's programmes by age | L | 1.3.0, done 2026-10-10 |
| H-06 | The decoder reads a history: categories, long texts, the eras' timeline, the people's history as read | M | 1.3.0, done 2026-10-10 |

The three blocks were added on 2026-10-08 for 1.3.0 (R-411), interleaved with the far and the faint in the iteration's
order (H-01, A-01, then the radar, A-02 to A-04, T-01, H-02, T-02 to T-04, H-03 to H-06); one generation change for the
three, from A-01. They rebuild what the C series made (C-01's settlements, C-02's
grammar, C-03's shards, C-08's reading of the site, C-09's roads, C-12's graves) and keep its register.

## V: the surface picture (`IDEAS-surface-picture.md`) — the rendering polish asked for after C-09

| Id | Item | Size | Status |
| --- | --- | --- | --- |
| V-01 | Continuous LOD: nested rings geomorphed everywhere, one shading and one set of material tiles at every distance, no cell popping, no shore steps | XL | 1.4.0 |
| V-02 | Normals from the planet function's gradient, ambient by slope and sky, a finer intensity ramp | M | 1.4.0 |
| V-03 | Aerial perspective: height-aware per-pixel fog shared by every pass, a horizon haze band, scattering toward the sun | M | 1.4.0 |
| V-04 | Flora and rock LOD: distance-faded instancing, billboards far out, forests that do not shimmer | L | 1.4.0 |
| V-05 | Ground texture to the horizon: the meso tiles at coarser octaves on every ring, the macro tone at all scales | M | 1.4.0 |
| V-06 | Water and sky: shoreline foam, reflections by the wave normal, a cloud layer with parallax | L | 1.4.0 |
| V-07 | The artifact audit: every site, the stability sheets, the holes and seam checks; KI-310/318/320/323/341 closed or re-scoped | S | 1.4.0 |

V-01 first: everything else sits on it; V-02 and V-03 next, cheap and visible in every frame. Parked on 2026-10-03: the direction (this series, or GPU rendering through raylib) is to be discussed before any of it starts.

## W: the world and the sky (`IDEAS-world-and-sky.md`)

| Id | Item | Size | Status |
| --- | --- | --- | --- |
| W-01 | The time-lapse as an almanac menu: upcoming events with countdowns, run-to-it with a slowdown at the end; orbital events on the ship | M | 1.2.0, done 2026-10-06 |
| W-02 | Tides: water level follows the moon and sun, tidal range from masses, tide line on the maps | M | 1.4.0, as Q-02's great tides |
| W-03 | Weather that arrives: fronts with position, heading and speed; visible from orbit, on the map and on the ground; the data sheet forecast | L | 1.2.0, done 2026-10-06 |
| W-04 | Nights: aurorae, meteor showers from comet orbits, zodiacal light, ring and moon shadows, earthshine | M | 1.2.0, done 2026-10-07 |
| W-05 | Fossils and strata: layer sequences from the world's history, an exposure landmark kind, the data sheet's reading, fossils on living worlds | L | 1.4.0 |
| W-06 | The telescope on the ship: zoom, reticle, drift, photo mode | M | 1.2.0, done 2026-10-06, reworked the same day on review (the plate, the set, R-404), then R-405 (the stabiliser, the orbit keys) |

W-01 first: it makes every other timed event (W-02..W-04) something you can witness.

## P: kinds of world (`IDEAS-worlds-and-wanderers.md` section 1) — generation 16 from P-01

Re-cut on 2026-10-21 for 1.4.0 (R-421): the first list of `IDEAS-planet-kinds-and-looks.md` had twelve rows, one a kind, none
started; its kinds are in the families below (the eyeball and the ammonia world in P-03, the salt, glass and chthonian worlds
and the supercritical one as the steam world's lowlands in P-01, the fungal world as P-03's chemotroph, the rogue-captured
world as P-05's wanderer, the shattered world as P-06's patchwork, the iron rain as P-07's silicate giant); the shepherd moon
(a place) and the pelagic island (a trait) are left for later.

| Id | Item | Size | Status |
| --- | --- | --- | --- |
| P-00 | The kind table: every kind a row, the planet function's shared skeleton, accent hues, traits by tags; nothing drawn changes | XL | 1.4.0, done 2026-10-21 |
| P-01 | The furnace worlds: magma ocean, chthonian, glass, steam, salt, rust; the new kinds from the old draws; generation 16 | L | 1.4.0, done 2026-10-21 |
| P-02 | The living worlds: archean, hothouse, superhabitable, steppe, tundra, lichen | L | 1.4.0, done 2026-10-21 |
| P-03 | The locked and the deep: eyeball, terminator, hycean, ammonia, snowball, chemotroph | L | 1.4.0, done 2026-10-21 |
| P-04 | The dead worlds and their peoples: frozen felisian, scorched felisian | L | 1.4.0, done 2026-10-21 |
| P-05 | The far cold: nitrogen, cryovolcanic, dry ice, wanderer, brine dwarf, frost dwarf | L | 1.4.0, done 2026-10-22 |
| P-06 | The strange moons: grooved ice, two-faced, tiger-striped, patchwork, sponge | M | 1.4.0, done 2026-10-22 |
| P-07 | The giants: water-cloud, azure, alkali, silicate, gas dwarf, super-puff; the probe's decks | L | 1.4.0, done 2026-10-22 |

## Q: the qualities (`IDEAS-worlds-and-wanderers.md` section 2)

| Id | Item | Size | Status |
| --- | --- | --- | --- |
| Q-01 | Six slots a world, the facts, eleven landforms | L | 1.4.0, done 2026-10-22 |
| Q-02 | Water, sky, life and ground: sixteen traits, the tides among them (W-02) | L | 1.4.0, done 2026-10-22 |
| Q-03 | The sounds of the ground: singing dunes, singing ice, quakes | M | 1.4.0, done 2026-10-22 |

## D: the drifters (`IDEAS-worlds-and-wanderers.md` section 3)

| Id | Item | Size | Status |
| --- | --- | --- | --- |
| D-01 | The dark between: the drifters' field, a system with no sun, the way there and back, the save | L | 1.4.0, done 2026-10-23 |
| D-02 | Heard, not seen: the radar finds them, the lock, the jump into the void, the black disc, the map without light | L | 1.4.0, done 2026-10-23 |
| D-03 | The voices of the dark: a voice for every drifter | L | 1.4.0, done 2026-10-23 |
| D-04 | The ground in the dark: the explorer's lamp, frozen air, embers, the lights left on, the dark ocean | L | 1.4.0, done 2026-10-23 |
| D-05 | The people who lost their sun: the history's new end, its records, charts and music, the beacon | XL | 1.4.0, done 2026-10-23 |

## L: looks and effects (`IDEAS-planet-kinds-and-looks.md`, second list)

| Id | Item | Size | Status |
| --- | --- | --- | --- |
| L-01 | Atmospheric scattering with distance and altitude | L | 1.4.0 or later |
| L-02 | Cloud shadows moving over the ground | M | 1.4.0 or later |
| L-03 | Wet ground after rain: darker material, puddles, gullies running | M | 1.4.0 or later |
| L-04 | Snow and frost by time of day and season | M | 1.4.0 or later |
| L-05 | Dust and sand in the air: haze, streaming crests, deposits | M | 1.4.0 or later |
| L-06 | Material detail by distance: pebbles, cracked mud, ripples, moss | L | 1.4.0 or later |
| L-07 | Volumetric light: rays, shafts, plume glow | L | 1.4.0 or later |
| L-08 | Wind on vegetation and water | M | 1.4.0 or later |
| L-09 | Heat shimmer | S | 1.4.0 or later |
| L-10 | Bioluminescence as a trait | M | 1.4.0 or later |
| L-11 | Ring light and planetshine on the ground | M | 1.4.0 or later |
| L-12 | Palette moods per world | S | 1.4.0 or later |
| L-13 | Weathering on the capsule and the buggy | M | 1.4.0 or later |
| L-14 | Ground fog and mist | M | 1.4.0 or later |

L-01 and L-02 first: they change every frame on every world.

## X: the gas giant probe (`IDEAS-gas-giant-probe.md`)

| Id | Item | Size | Status |
| --- | --- | --- | --- |
| X-01 | Launch and relay flow from orbit, the cabin screen, aiming, the parachute stage, the end | M | 1.2.0, done 2026-10-07 |
| X-02 | The descent's stages: above the clouds, haze, the ammonia deck as terrain, between decks, the water deck | XL | 1.2.0, done 2026-10-07 (with R-406, Shift runs the probe x8) |
| X-03 | Per-giant character: decks, storms, lightning, glow, aurorae, rare features; brown dwarfs and ice giants | M | 1.2.0, done 2026-10-07 (generation 12) |
| X-04 | What comes back: the recording in the gallery, the final image, the depth profile on the data sheet, log events and statistics, export | M | 1.2.0, done 2026-10-07 |

## K: caves (`IDEAS-caves.md`) — after the G/S bump

| Id | Item | Size | Status |
| --- | --- | --- | --- |
| K-01 | Design answers to the open questions (finding, darkness, life, the buggy, the map) | S | 1.4.0 or later |
| K-02 | Placement: `LM_CAVE` through the landmark grid; sinkholes and springs from the drainage on karst worlds; lava-tube, sea, ice and collapse variants | M | 1.4.0 or later |
| K-03 | Geometry: the cut into the terrain rings at the mouths, the tunnel mesh and its collision | XL | 1.4.0 or later |
| K-04 | Rendering: daylight falloff, the helmet lamp, floor water, skylights, sound | L | 1.4.0 or later |
| K-05 | Maps: the sinkhole and spring on the landing zoom, the walked passage on the sector map | S | 1.4.0 or later |

## F: the frame (`PLAN-performance.md`) — faster without changing a pixel, no generation change

Planned 2026-10-12 from a day of measurements and an audit of every subsystem; the plan holds the verified findings, the
targets and the method. Every item but F-10 is bit-identical (the 22 regress hashes stay); F-10 is one blessed batch.
The audit's own measurement found B-423 (the forest's picture depends on the core count: the flora's coverage grid is
per band), which is fixed before any band-boundary work (F-05, F-06); the plan's section 4 has the two ways. Progress (2026-10-15):
F-01 done; B-423 done (the pinned bands; its timing pairs and flow frame closed 2026-10-13); F-02 done; F-03 done; F-04 done; F-05 done; F-06 done; F-07 done; F-08 done; F-09 done; F-10 done; F-11 done: the series is closed, released as 1.3.1 on 2026-10-15.

| Id | Item | Size | Status |
| --- | --- | --- | --- |
| F-01 | Measure what the bench cannot see: the real game's frame sections, `pngdiff`, serial-versus-workers checks for the cabin, the forest and the feeds, the per-band terrain trace, the pool and noise micro-benches, rows for the landing map, a sun-facing frame, the radar camera and the guide save | M | 1.3.1, done 2026-10-12 (Linux and Windows runs open) |
| F-02 | The stalls in play: the guide's probe lines cached, the PNG encoder and a writer thread, the trees against a settlement's buildings, the direction table in parallel, the star neighbourhood walked incrementally, the landing probe's `initFrame`; the landing map and the drone bay open at once on a world not mapped yet (the user's choice) | M | 1.3.1, done 2026-10-13 |
| F-03 | The first frame of a descent and the tiles: the rings' misses filled by the pool, the tile's integer-key sort; the breach's two-cell pits sunk a binade at a time; the landing's textures and meso tiles on the pool; the drainage's neighbourhood made from its cell (the user's choice) | M | 1.3.1, done 2026-10-13 (its timing batch on mains power 2026-10-14: the space rows within 1%) |
| F-04 | The terrain rings: cells binned by band once, water corners per vertex, the warm pass's vertex rows (the adaptive row chunks after F-06) | L | 1.3.1, done 2026-10-14 |
| F-05 | The passes one thread draws alone, in bands: the cabin, the maps' and menus' blits, `glowDisc`, the camera feeds, the sky grids, the floor and reflections, the sun's cone test (and KI-409, the sun's loop and the stack) | M | 1.3.1, done 2026-10-14 |
| F-06 | The pool deals its rows: atomic chunks, a bounded spin, the reentrancy guard, the terrain's adaptive chunks; the cap raise measured | M | 1.3.1, done 2026-10-14 (the spin, the chunks by cost and the cap of ten measured and not taken) |
| F-07 | Exact trims: `wrap2pi`, the map accessors inline, the globe's trims, the mush, the raster, the sky's skips, the halo and pillar, the plate, the probe, the noise table, the synth, `ifloor`; the backdrop replay | M | 1.3.1, done 2026-10-14 (in five A/B steps; `ifloor`'s truncation on baseline x86-64 only, the toRGB cache and the ring-shadow skip not made) |
| F-08 | The platform layer: the audio block on a helper thread, the upload measured; the frames' pacing (done: the platform paces, the thread and the ring dropped) | S | 1.3.1, done |
| F-09 | The release flags: thin LTO on the clang lines, the build scripts into `packaging/`, the Linux and Windows flags gated on the container's regress | M | 1.3.1, done 2026-10-15 (thin LTO on macOS and Windows; Linux `-O3`, its `-flto` slower and `-fno-math-errno` nothing in the container; Windows untimed; B-424 found by the Windows builds) |
| F-10 | The hash-moving batch, one bless: the sky's pows as products, the cloud drift wrapped once, the limb, the `atan2`/`asin` pair | M | 1.3.1, done 2026-10-15 (the sky's powers as products, the cloud drift wrapped once, the limb as products: surface 4x -14%, space 4x -8%; no hash moved, nothing blessed; the `atan2`/`asin` pair built and dropped, slower in the frame) |
| F-11 | The close: budgets lowered, the regress widened, the docs and KNOWN-ISSUES | S | 1.3.1, done 2026-10-15 (37 budgets at about 1.5 times the dev Mac's medians, `bench check <factor>` elsewhere; the regress 24: the forest at 4x and the cabin at 1x, each drawn by one thread too; KI-312 and KI-206 closed) |

## Suggested order for 1.2.0

1.1.0 shipped the G and S bump, the C series but C-11, and the aurora's two items. For 1.2.0:

1. The V series (the surface picture: V-01 the continuous LOD first, then V-02, V-03, V-04, V-05,
   V-06, V-07), the rendering polish the user asked for after C-09.
2. W-01 (the almanac), then W-02 and W-04: cheap and visible on every world.
3. C-11 (the instruments) once KI-347 is settled; the texts of C-02 once judged on paper.
4. Then by appetite: the probe (X), caves (K), the planet kinds (P) and the looks (L).

## 1.3.0

The iteration is `ITERATION-1.3.0.md` (2026-10-08), one release in twenty milestones and eight phases: the far (G-05,
G-06, G-07), groundwork (H-01, A-01), the faint (C-15, C-16), what they built (A-02, A-03, A-04), where and when (T-01,
H-02), towns and works (T-02, T-03, T-04), the records (H-03, H-04, H-05, H-06), the player (C-11). The V series stays parked until
the rendering direction is discussed; W-02, W-05, S-07 and the P, L and K series wait for a later iteration.
