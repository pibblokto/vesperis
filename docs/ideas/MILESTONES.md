# Milestones for 1.1.0 and beyond

The ideas in this directory, cut into milestones with ids to reference in requests, bug
reports and `PROGRESS.md` ("proceed with C-02"). Each item names the section of its
document. Sizes: S (an afternoon), M (a day or two), L (a week), XL (a milestone of its own).
Nothing here is started; order is a suggestion.

## G: the galaxy (`PLAN-galaxy-scale.md`) — one generation bump with S

| Id | Item | Size |
| --- | --- | --- |
| G-01 | New density constants: ~200 billion stars, Milky Way proportions, local density kept | M |
| G-02 | Regions, globular clusters, start sector and home star in the new disc | S |
| G-03 | Galaxy background integration at the new scale (two-scale ray integration) | M |
| G-04 | Scene finders and pinned sites re-found, `regress bless`, flow green | S |

## S: star classes (`PLAN-star-classes.md`) — same bump as G

| Id | Item | Size |
| --- | --- | --- |
| S-01 | Plain rows: red dwarf (flares, tidal locking), blue-white, orange giant, carbon star; region weights | M |
| S-02 | UI seams: class filter beyond six bits, codes S06+, "classes seen" from the table, descriptions | S |
| S-03 | Neutron star (debris system, glassed worlds) | M |
| S-04 | Protostar (nebula sky, dust and belts, accretion disc) | M |
| S-05 | Wolf-Rayet star (shell ring in the sky, stripping wind, radiation) | M |
| S-06 | Black hole: lensing, accretion ring, drawn-out companion, survivor rocks | XL |
| S-07 | Later rows when wanted: yellow supergiant, magnetar, T Tauri, white dwarf pair | S each |

## C: civilisations (`IDEAS-ruins-shards-signals.md`)

| Id | Item | Size |
| --- | --- | --- |
| C-01 | The "had a civilisation" trait on desert and felisian worlds; ruin density and a second ruin style; post-apocalyptic deserts keep dry seabeds and old rivers | M |
| C-02 | Story grammar prototype as a harness mode printing fifty shards per world; vocabulary from the world's data; recurring proper nouns; tone by world type | L |
| C-03 | Shards as objects in ruins: placement, finding, the log and the guide statistics | M |
| C-04 | Music: per-world tradition (scale, cycle, timbres) and generated pieces; name a piece | L |
| C-05 | Language: phoneme inventory, prosody, voice synthesis; alien speech aligned with the decoded text | L |
| C-06 | Decoding on the ship: the cabin screen, the wait, the decoded text; a language you learn (30% first, re-decoding as shards accumulate) | M |
| C-07 | The signal radar: galaxy-hashed signals, the sweep, lock, distance and age, rare and far; non-civilisation signals (pulsar, comet, magnetosphere) | L |
| C-08 | Ruins that read the world: harbours, terraces, cisterns and walls from the terrain; weathering by age | L |
| C-09 | Roads that still go somewhere: faint lines in the terrain, on the landing zoom, between ruins | M |
| C-10 | Star charts as shards: the culture's sky overlaid on yours, pointing at other dead worlds | M |
| C-11 | Instruments as artefacts, playable from the cabin | S |
| C-12 | Graves and names; a culture's calendar and the timeline; the last recording per world | M |
| C-13 | Cultures per world: none, one or (rarely) two | S |
| C-14 | Lending: decoded shards travel with the guide export, credited in cyan | S |

Order: C-01, C-02 (judge the text before any audio), C-03, C-06, C-04, C-05, C-07, then
C-08 and C-10 as the pair that connects the galaxy, the rest as wanted.

## W: the world and the sky (`IDEAS-world-and-sky.md`)

| Id | Item | Size |
| --- | --- | --- |
| W-01 | The time-lapse as an almanac menu: upcoming events with countdowns, run-to-it with a slowdown at the end; orbital events on the ship | M |
| W-02 | Tides: water level follows the moon and sun, tidal range from masses, tide line on the maps | M |
| W-03 | Weather that arrives: fronts with position, heading and speed; visible from orbit, on the map and on the ground; the data sheet forecast | L |
| W-04 | Nights: aurorae, meteor showers from comet orbits, zodiacal light, ring and moon shadows, earthshine | M |
| W-05 | Fossils and strata: layer sequences from the world's history, an exposure landmark kind, the data sheet's reading, fossils on living worlds | L |
| W-06 | The telescope on the ship: zoom, reticle, drift, photo mode | M |

W-01 first: it makes every other timed event (W-02..W-04) something you can witness.

## P: planet kinds (`IDEAS-planet-kinds-and-looks.md`, first list) — each is a generation change

| Id | Item | Size |
| --- | --- | --- |
| P-01 | Tidally locked eyeball world (with S-01's red dwarfs) | L |
| P-02 | Ammonia world | M |
| P-03 | Salt world | M |
| P-04 | Glass world | M |
| P-05 | Fungal world | L |
| P-06 | Rogue-captured world | M |
| P-07 | Chthonian world | M |
| P-08 | Shattered world | L |
| P-09 | Supercritical world (landable at the poles or not at all) | M |
| P-10 | Iron rain world | M |
| P-11 | Shepherd moon in a ring (a place, not a type) | M |
| P-12 | Pelagic world with a single island | S |

## L: looks and effects (`IDEAS-planet-kinds-and-looks.md`, second list)

| Id | Item | Size |
| --- | --- | --- |
| L-01 | Atmospheric scattering with distance and altitude | L |
| L-02 | Cloud shadows moving over the ground | M |
| L-03 | Wet ground after rain: darker material, puddles, gullies running | M |
| L-04 | Snow and frost by time of day and season | M |
| L-05 | Dust and sand in the air: haze, streaming crests, deposits | M |
| L-06 | Material detail by distance: pebbles, cracked mud, ripples, moss | L |
| L-07 | Volumetric light: rays, shafts, plume glow | L |
| L-08 | Wind on vegetation and water | M |
| L-09 | Heat shimmer | S |
| L-10 | Bioluminescence as a trait | M |
| L-11 | Ring light and planetshine on the ground | M |
| L-12 | Palette moods per world | S |
| L-13 | Weathering on the capsule and the buggy | M |
| L-14 | Ground fog and mist | M |

L-01 and L-02 first: they change every frame on every world.

## X: the gas giant probe (`IDEAS-gas-giant-probe.md`)

| Id | Item | Size |
| --- | --- | --- |
| X-01 | Launch and relay flow from orbit, the cabin screen, aiming, the parachute stage, the end | M |
| X-02 | The descent's stages: above the clouds, haze, the ammonia deck as terrain, between decks, the water deck | XL |
| X-03 | Per-giant character: decks, storms, lightning, glow, aurorae, rare features; brown dwarfs and ice giants | M |
| X-04 | What comes back: the recording in the gallery, the final image, the depth profile on the data sheet, log events and statistics, export | M |

## K: caves (`IDEAS-caves.md`) — after the G/S bump

| Id | Item | Size |
| --- | --- | --- |
| K-01 | Design answers to the open questions (finding, darkness, life, the buggy, the map) | S |
| K-02 | Placement: `LM_CAVE` through the landmark grid; sinkholes and springs from the drainage on karst worlds; lava-tube, sea, ice and collapse variants | M |
| K-03 | Geometry: the cut into the terrain rings at the mouths, the tunnel mesh and its collision | XL |
| K-04 | Rendering: daylight falloff, the helmet lamp, floor water, skylights, sound | L |
| K-05 | Maps: the sinkhole and spring on the landing zoom, the walked passage on the sector map | S |

## Suggested order for 1.1.0

1. G and S together (the generation bump), before anyone keeps saves on 1.0.0 for long.
2. W-01 (the almanac), then W-02 and W-04: cheap and visible on every world.
3. C-01 and C-02: the civilisation trait and the text, judged on paper before the rest.
4. Then by appetite: the probe (X), the shards' audio (C-04..C-06), the radar (C-07), caves (K).
