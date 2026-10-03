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
column: `1.1.0` shipped in that release, `1.2.0` is for the next one.

**Left for 1.2.0 and later:** C-11 (skipped under KI-347), S-07, the W, P, L, X and K series.
Two additions of the user's are in `IDEAS-ruins-shards-signals.md` section 5 (ruins with a
lot more variety and interiors; not every desert is post-apocalyptic).

## G: the galaxy (`PLAN-galaxy-scale.md`) — one generation bump with S

| Id | Item | Size | Status |
| --- | --- | --- | --- |
| G-01 | New density constants: ~200 billion stars, Milky Way proportions, local density kept | M | 1.1.0 |
| G-02 | Regions, globular clusters, start sector and home star in the new disc | S | 1.1.0 |
| G-03 | Galaxy background integration at the new scale (two-scale ray integration) | M | 1.1.0 |
| G-04 | Scene finders and pinned sites re-found, `regress bless`, flow green | S | 1.1.0 |

## S: star classes (`PLAN-star-classes.md`) — same bump as G

| Id | Item | Size | Status |
| --- | --- | --- | --- |
| S-01 | Plain rows: red dwarf (flares, tidal locking), blue-white, orange giant, carbon star; region weights | M | 1.1.0 |
| S-02 | UI seams: class filter beyond six bits, codes S06+, "classes seen" from the table, descriptions | S | 1.1.0 |
| S-03 | Neutron star (debris system, glassed worlds) | M | 1.1.0 |
| S-04 | Protostar (nebula sky, dust and belts, accretion disc) | M | 1.1.0 |
| S-05 | Wolf-Rayet star (shell ring in the sky, stripping wind, radiation) | M | 1.1.0 |
| S-06 | Black hole: lensing, accretion ring, drawn-out companion, survivor rocks | XL | 1.1.0 |
| S-07 | Later rows when wanted: yellow supergiant, magnetar, T Tauri, white dwarf pair | S each | 1.2.0 or later |

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
| C-11 | Instruments as artefacts, playable from the cabin | S | 1.2.0 (KI-347) |
| C-12 | Graves and names; a culture's calendar and the timeline; the last recording per world | M | 1.1.0 |
| C-13 | Cultures per world: none, one or (rarely) two | S | 1.1.0 |
| C-14 | Lending: decoded shards travel with the guide export, credited in cyan | S | 1.1.0 |

Order: C-01, C-02 (judge the text before any audio), C-03, C-06, C-04, C-05, C-07, then
C-08 and C-10 as the pair that connects the galaxy, the rest as wanted.

## W: the world and the sky (`IDEAS-world-and-sky.md`)

| Id | Item | Size | Status |
| --- | --- | --- | --- |
| W-01 | The time-lapse as an almanac menu: upcoming events with countdowns, run-to-it with a slowdown at the end; orbital events on the ship | M | 1.2.0 |
| W-02 | Tides: water level follows the moon and sun, tidal range from masses, tide line on the maps | M | 1.2.0 |
| W-03 | Weather that arrives: fronts with position, heading and speed; visible from orbit, on the map and on the ground; the data sheet forecast | L | 1.2.0 |
| W-04 | Nights: aurorae, meteor showers from comet orbits, zodiacal light, ring and moon shadows, earthshine | M | 1.2.0 |
| W-05 | Fossils and strata: layer sequences from the world's history, an exposure landmark kind, the data sheet's reading, fossils on living worlds | L | 1.2.0 |
| W-06 | The telescope on the ship: zoom, reticle, drift, photo mode | M | 1.2.0 |

W-01 first: it makes every other timed event (W-02..W-04) something you can witness.

## P: planet kinds (`IDEAS-planet-kinds-and-looks.md`, first list) — each is a generation change

| Id | Item | Size | Status |
| --- | --- | --- | --- |
| P-01 | Tidally locked eyeball world (with S-01's red dwarfs) | L | 1.2.0 or later |
| P-02 | Ammonia world | M | 1.2.0 or later |
| P-03 | Salt world | M | 1.2.0 or later |
| P-04 | Glass world | M | 1.2.0 or later |
| P-05 | Fungal world | L | 1.2.0 or later |
| P-06 | Rogue-captured world | M | 1.2.0 or later |
| P-07 | Chthonian world | M | 1.2.0 or later |
| P-08 | Shattered world | L | 1.2.0 or later |
| P-09 | Supercritical world (landable at the poles or not at all) | M | 1.2.0 or later |
| P-10 | Iron rain world | M | 1.2.0 or later |
| P-11 | Shepherd moon in a ring (a place, not a type) | M | 1.2.0 or later |
| P-12 | Pelagic world with a single island | S | 1.2.0 or later |

## L: looks and effects (`IDEAS-planet-kinds-and-looks.md`, second list)

| Id | Item | Size | Status |
| --- | --- | --- | --- |
| L-01 | Atmospheric scattering with distance and altitude | L | 1.2.0 or later |
| L-02 | Cloud shadows moving over the ground | M | 1.2.0 or later |
| L-03 | Wet ground after rain: darker material, puddles, gullies running | M | 1.2.0 or later |
| L-04 | Snow and frost by time of day and season | M | 1.2.0 or later |
| L-05 | Dust and sand in the air: haze, streaming crests, deposits | M | 1.2.0 or later |
| L-06 | Material detail by distance: pebbles, cracked mud, ripples, moss | L | 1.2.0 or later |
| L-07 | Volumetric light: rays, shafts, plume glow | L | 1.2.0 or later |
| L-08 | Wind on vegetation and water | M | 1.2.0 or later |
| L-09 | Heat shimmer | S | 1.2.0 or later |
| L-10 | Bioluminescence as a trait | M | 1.2.0 or later |
| L-11 | Ring light and planetshine on the ground | M | 1.2.0 or later |
| L-12 | Palette moods per world | S | 1.2.0 or later |
| L-13 | Weathering on the capsule and the buggy | M | 1.2.0 or later |
| L-14 | Ground fog and mist | M | 1.2.0 or later |

L-01 and L-02 first: they change every frame on every world.

## X: the gas giant probe (`IDEAS-gas-giant-probe.md`)

| Id | Item | Size | Status |
| --- | --- | --- | --- |
| X-01 | Launch and relay flow from orbit, the cabin screen, aiming, the parachute stage, the end | M | 1.2.0 or later |
| X-02 | The descent's stages: above the clouds, haze, the ammonia deck as terrain, between decks, the water deck | XL | 1.2.0 or later |
| X-03 | Per-giant character: decks, storms, lightning, glow, aurorae, rare features; brown dwarfs and ice giants | M | 1.2.0 or later |
| X-04 | What comes back: the recording in the gallery, the final image, the depth profile on the data sheet, log events and statistics, export | M | 1.2.0 or later |

## K: caves (`IDEAS-caves.md`) — after the G/S bump

| Id | Item | Size | Status |
| --- | --- | --- | --- |
| K-01 | Design answers to the open questions (finding, darkness, life, the buggy, the map) | S | 1.2.0 or later |
| K-02 | Placement: `LM_CAVE` through the landmark grid; sinkholes and springs from the drainage on karst worlds; lava-tube, sea, ice and collapse variants | M | 1.2.0 or later |
| K-03 | Geometry: the cut into the terrain rings at the mouths, the tunnel mesh and its collision | XL | 1.2.0 or later |
| K-04 | Rendering: daylight falloff, the helmet lamp, floor water, skylights, sound | L | 1.2.0 or later |
| K-05 | Maps: the sinkhole and spring on the landing zoom, the walked passage on the sector map | S | 1.2.0 or later |

## Suggested order for 1.2.0

1.1.0 shipped the G and S bump, the C series but C-11, and the aurora's two items. For 1.2.0:

1. The rendering polish the user asked about after C-14 (continuous LOD, flora LOD, analytic
   normals, aerial perspective, tiles to the horizon, water and sky), to be cut into milestones.
2. W-01 (the almanac), then W-02 and W-04: cheap and visible on every world.
3. C-11 (the instruments) once KI-347 is settled; the texts of C-02 once judged on paper.
4. Then by appetite: the probe (X), caves (K), the planet kinds (P) and the looks (L).
