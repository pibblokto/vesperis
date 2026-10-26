# Roadmap, second iteration

**Status (2026-09-26): iteration 2 complete except N5-04 (blocked: no emsdk on this machine and the tree is not on GitHub). N0-N4 done (comets animate; one material palette drives globe, map and ground; forests with seven tree silhouettes, undergrowth per biome, seasons and sound; a bestiary per world with articulated animals, behaviours, calls and sightings; a real buggy with a cabin, 115 km/h and effects); N5-01/02/03/05 done (nebula patches, sun pillars and visor glints, evolving storms, the key bindings screen).** Built from four complaints: comets look frozen (B-201), the surface does not match the orbital view (B-202), felisian worlds are weak in vegetation and animals (R-201, R-202), the buggy looks and drives poorly (R-203).

## Where we are

The first iteration delivered the whole loop (galaxy, systems, landings, ship interior, guide, movement, generation, astronomy, presentation, tooling), verified through `vesperis_test`. What it did not deliver is *conviction on the ground*: a landing looks like a landing but the world you walk on is only loosely the world you saw from orbit, life is a token gesture (blob trees, box animals), and the buggy is a placeholder shape. The engine can carry more: 16 palette banks, meso textures, cast and blob shadows, point sprays, a raster budget of about 3 ms per frame at 2x with 12 ms to spare.

## Where we can get

A felisian landing that feels alive at every distance: from orbit a green-and-tan world with the forests, deserts and snow you will find on the ground; on the way down the landing map's colours become the terrain's; on the ground dense canopies with shade under them, meadows and reeds, a herd of something grazing by the water that lifts its heads as you come, birds settling in the trees at dusk; and a buggy you climb into, with the cage and the hood in front of you, that runs at 110 km/h across the plain and leaves dust. Comets that stream. Everything still one deterministic function of position and time, still the mush look.

Suggested order: N0 (the quick fix) -> N1 (one palette for the three views; N2 builds on it) -> N2 (vegetation) -> N3 (creatures) -> N4 (the buggy; independent, can be pulled earlier) -> N5 (leftovers). Engineering health (N6) rides along: every item ships with a headless check and the docs updated.

## N0 — Bug fixes first

| Id | Item | Size | Depends | Acceptance |
| --- | --- | --- | --- | --- |
| N0-01 | B-201 comets: streaming tail knots, coma flicker, a curved dust tail along the orbit, comet parking at 60 radii, speed and periapsis readouts, current distance in the analyzer | M | - | two `space` frames 5 s apart differ; at x100 the comet drifts more than 4 px in 500 s; `shots/space_comet_close.png` |
| | **done 2026-09-26**: knots, dust tail, jets close up; parked at 12 radii (60 left the nucleus 8 px wide); HUD, data sheet and analyzer readouts; `space` and `flow` checks | | | |
| N0-02 | B-202 is a milestone (N1); here only the diagnosis sheet: `vesperis_test consistency` renders map crop, globe patch and ground view side by side for the 13 landable types and prints the colour distances (the numbers N1 must bring down) | S | - | `shots/consistency_<TYPE>.png`, a table in `08-testing-and-tools.md` |
| | **done 2026-09-26**: 11 of 13 types over 60, worst 230 (icy), the table is in B-202 and `08-testing-and-tools.md` | | | |

## N1 — What you see from orbit is what you land on (see `PLAN-orbit-to-surface-consistency.md`)

Goal: one material palette per body drives the globe, the landing map and the ground.

| Id | Item | Size | Depends | Acceptance |
| --- | --- | --- | --- | --- |
| N1-01 | `BodyGen::matColor[MAT_COUNT]`: per-material colours derived from the body hue (`Body::color`), per-material tints, the star's light, biome and season; `vegColor`, `skyTint` and the landing map table fold into it | M | - | unit test: every material of every type gets a colour; the palette is deterministic per seed |
| N1-02 | Globe with per-material banks: body A gets rock (2), water (6), vegetation (8), sand/desert (9), snow/ice (10); body B keeps 3 and 7 with the nearest of the shared banks; lava, gas and substellar ramps unchanged; the map stores the material already, so no generation change | M | N1-01 | `space` globe frames show forests, deserts and ice caps in their own colours; regress frame hashes re-blessed with the reason logged |
| N1-03 | Surface ramps from the same palette: material -> bank table extended (sand, dust, ice, snow, metal, sulphur, graphite get banks or tints instead of sharing the ground bank), the hemisphere-light and sunset tints kept; `lookFor` keeps only sky, fog and night colours | M | N1-01 | `surface` frames per type; the ground colour at noon within the threshold of the map cell |
| N1-04 | Landing map from the palette (drop `matCol`), plus a "what you will see" strip: the ground colour, the material name and the biome at the cursor, drawn with the surface ramp | S | N1-01 | `landmaps` frames |
| N1-05 | Brightness match: the globe's shading at the sub-solar point and the ground's noon shade map to the same palette stop (today the globe runs 63 x albedo^0.6, the ground its own curve); one exposure rule documented in `02-rendering.md` | S | N1-02, N1-03 | the consistency test's distances under the threshold for all 13 types |
| N1-06 | Character match beyond colour: the globe's relief shading strength follows the map's real slopes; the landing map readout shows relief (`FLAT / HILLS / MOUNTAINS`) and the surface delivers it (already true, now measured) | S | - | the readout's class matches the measured slope at the site in the test |
| | **N1 done 2026-09-26**: `BodyGen::matColor` + six material families with banks in both renderers (N1-01/02/03), the landing map from the same ramp with a ground swatch and a relief class (N1-04/06), one exposure rule `exposureStop` and one ramp `materialRamp` (N1-05); `consistency` 0 of 13 over the threshold (venusian map-to-ground only), unit checks for the palette, regress re-blessed; outcome in `bugs/resolved/B-202-*.md` | | | |

## N2 — Living worlds II: vegetation (see `PLAN-living-worlds-2.md`)

Goal: forests, meadows and wetlands that read as such at every distance, distinct per biome and per world.

| Id | Item | Size | Depends | Acceptance |
| --- | --- | --- | --- | --- |
| N2-01 | Tree archetypes rebuilt: trunk with 2-4 branches, canopy as 8-20 overlapping leaf blobs in three shades (lit crown, mid, shaded underside) with per-tree hue jitter; seven silhouettes (dome, cone, umbrella, tiered giant, fibrous stalk, fern tree, mushroom tree); two per world plus biome overrides | L | N1-01 | `scene felisian_forest` at 1x-4x; canopy coverage in forest biome over 60% within 200 m |
| N2-02 | Forest structure: density and height from `veg` and moisture, clearings from a noise mask, edge trees smaller, dead trees and fallen logs (colliders), forest floor darker and mossy under canopy (ground colour shift from canopy density), leaf litter grain | M | N2-01 | frames; walking under canopy is visibly darker |
| N2-03 | Undergrowth and meadows: ferns, bushes with berries (colour points), tall swaying grass patches within 60 m, flowers as colour points in grassland and savanna, reeds and lily pads at water edges, cattail-like stalks in wetlands, cacti and succulents in deserts, lichen and cushions on tundra, kelp seen through shallow water | L | N2-01 | one scene per biome (`scene biome_<name>`) |
| N2-04 | Distance: canopy impostors (2-4 blobs) to 1.5 km, single dots beyond, terrain colour carrying the forest beyond that (already), tree cast shadows and blob shadows sized by the canopy; performance budget: a forest view at 2x under 6 ms | M | N2-01 | `bench` forest scene under budget |
| N2-05 | Colour and season: per-family palettes (greens, blue-greens, purples, reds), autumn/winter recolouring from `season`, flowering season; night: canopies black against the sky, fireflies in wetlands at dusk | S | N2-01 | seasonal frames of one site |
| N2-06 | Sound and motion: wind through leaves scaled by canopy density, rustle when walking through undergrowth, canopy sway | S | N2-02 | `audio` stage "forest" |
| | **N2 done 2026-09-26** (`surface/flora.cpp`, outcome in `requests/R-201-*.md`): N2-01 seven silhouettes, three-tone blobs, branches, three leaf banks; coverage 92% tropical / 79% temperate. N2-02 clearing mask, edge trees, dead trees, logs as colliders, the floor darker under the canopy (the forest meso tile is the litter). N2-03 all listed but kelp (KI-207). N2-04 tiers near/mid (solid diamonds)/far (impostors), a coverage grid that skips trees hidden behind filled canopy tiles, a per-tree point budget; 12 ms at 2x in the densest tropical giant forest, budget 14 (KI-206, the 6 ms aim was not reached). N2-05 second family colour, autumn and bare winter, flowers in spring/summer, fireflies. N2-06 `audio.leaves`, canopy sway | | | |

## N3 — Living worlds II: creatures (see `PLAN-living-worlds-2.md`)

Goal: a bestiary per world with articulated, animated animals and behaviours worth watching.

| Id | Item | Size | Depends | Acceptance |
| --- | --- | --- | --- | --- |
| N3-01 | Bestiary generator: per felisian (and ocean) world 2-4 land species, 1-2 flyers, 1 swimmer, 0-1 nocturnal; each with a body plan (quadruped, long-necked browser, low hexapod crawler, hopper, biped strider, giant walker), size 0.3-8 m, gait, colours and pattern (stripes, spots, countershading) from the biome palette, a call, a name seed; listed on the data sheet and in the guide (nameable, "first seen" log entry, a creatures-seen stat) | L | N1-01 | `vesperis_test bestiary` prints species per world; determinism check |
| N3-02 | Articulated bodies: spine, neck, head, tail and legs as shaded segments (capsules of quads), eyes, ears/horns/crests as silhouettes; gait cycles per plan (walk, trot, gallop, hop, slither, crawl) driven by speed; idle, graze (head down/up), drink, rest, alert (head up, freeze), flee; flyers with flap and glide, landing on ground and trees; swimmers with fins breaking the surface and dives | XL | N3-01 | `scene felisian_herd` sheets at three distances; no limb clipping through the ground |
| N3-03 | Behaviours: herds with a leader and spacing, grazing paths that follow the grass and the water, drinking at water edges, resting at midday or at night (nocturnal species reverse), alert and flee with a call when the explorer comes within 25 m (stampede from the buggy), curious species approach to 10 m and stop, territorial giants that ignore you; flocks settle in trees at dusk and take off at dawn; insects/swarms near water at dusk | L | N3-02 | `flow` steps: approach a herd and log the reactions |
| N3-04 | Density and encounter design: something living within 200 m on 70% of temperate/tropical felisian landings, rarer in deserts and tundra, nothing on ice; the landing map's "what you will see" strip names the likely species; creature tracks in sand and snow | M | N3-01 | survey test over 300 felisian sites |
| N3-05 | Sounds: per-species calls (synthesised: chirps, lows, hoots, clicks), alarm calls, hoof/paw footsteps near, insects buzzing | S | N3-01 | `audio` stage "herd" |
| N3-06 | Highlight and info: `X` highlights with the species name and behaviour; a binocular zoom (`Z` held on foot?) to watch from afar | S | N3-02 | frames |
| | **N3 done 2026-09-26** (`surface/bestiary.*`, `surface/creatures.cpp`, outcome in `requests/R-202-*.md`): N3-01 bestiary per world, data sheet list, `SIGHTING` log entries, a creatures-sighted statistic (no naming by the explorer, KI-209). N3-02 segmented bodies with necks, heads, tails, jointed legs and six gaits, feet on the terrain; flyers flap, glide and perch; swimmers with backs and breaches. N3-03 the state machine with leaders, water walks, rest, alert, flight, stampedes from the buggy, curiosity, indifferent giants, perching at dusk, insects. N3-04 encounter chances per biome (`encounters` survey), the landing map's `LIFE:` name, tracks in sand and snow. N3-05 calls, hoof-steps, insects in the synth. N3-06 `X` names species and states; no zoom | | | |

## N4 — The buggy II (see `PLAN-buggy-2.md`)

Goal: a vehicle you recognise, sit in and enjoy at speed.

| Id | Item | Size | Depends | Acceptance |
| --- | --- | --- | --- | --- |
| N4-01 | Model: tubular roll cage, two bucket seats, angular body panels with fenders and a hood, four big knobby wheels on visible suspension arms that travel over bumps, wheels steering and spinning, headlights, tail lights, an antenna with a blinking light, a spare wheel; the ship's colour scheme with a bright chrome bank; drawn the same in the chase view, from the seat and when parked | L | - | frames parked, chase and seat; the folded/unfolding animation updated |
| N4-02 | Driving: top speed 32 m/s (115 km/h) forward, 8 m/s reverse, a torque curve (fast to 15 m/s, slow to the top), traction by material and gravity, handbrake skids, longer jumps on low gravity, speed-dependent camera shake, a hard cap on slopes; collisions push back and cost speed | M | - | `flow` drives 2 km at over 100 km/h without stalls; bench budget |
| N4-03 | Cabin from the seat: the cage tubes and windshield edge framing the view, the hood and fenders with the front wheels visible when steering, a steering wheel that turns with the input, gloved hands, a dashboard with dials (speed, heading, odometer, temperature, lights) replacing the HUD cowl, mirror optional; roll and pitch of the frame with the chassis | L | N4-01 | seat frames at rest, at speed, on a slope |
| N4-04 | Effects and sound: dust, sand or snow spray per wheel scaled by speed and material, mud splashes in wetlands, tracks matched to the wheels' width, engine pitch with gear steps, suspension and skid sounds, wind at speed, the headlight cones lighting the ground and trees | M | N4-01 | night frame with headlights; `audio` stage "buggy" |
| N4-05 | HUD and guide: the speedometer lives in the dashboard; the guide logs the longest drive and top speed; the buggy is drawn on the sector map | S | N4-03 | frames |
| N4-06 | The buggy III (R-204, B-203/204/205): a closed futuristic hull (sensor band, wheel pods, camera pod, light bars, mast, lidar puck) in the parked and chase views; from inside only the nose camera's picture with a CCTV look and its own interface; the camera pans +-70 deg and tilts -35..+25, never backwards; smooth tremor instead of random shake; no buggy carries over to a new landing, `K` always recalls the capsule, `B` at the capsule always unfolds a new buggy | M | N4 | `unit` state and camera checks; `flow` recall/redeploy/second landing; `drive`; scenes; bench |
| | **N4-06 done 2026-09-27** (`surface/buggy.cpp`, `surface_view.cpp`, `game/hud.cpp`; outcome in `requests/R-204-*.md`, reports in `bugs/resolved/B-203..205`). Bench 5.0 ms at 2x; `drive` 2,152 m at 107 km/h; regress unchanged | | | |
| | **N4 done 2026-09-26** (`surface/buggy.cpp`, outcome in `requests/R-203-*.md`): N4-01 the full model in all views, unfolding cage; N4-02 the torque curve to 115 km/h, traction, skids, bumps over small rocks, gears, thumps; N4-03 the cabin from the driver's seat with the wheel, hands, binnacle dials and hood; N4-04 spray, tread tracks, gears, skid and thumps in the synth, wind at speed (headlights still light the ground only, KI-210); N4-05 dashboard readouts, longest drive and top speed in the guide. `vesperis_test drive` 2.15 km at up to 107 km/h; bench 5.7 ms at 2x | | | |

## N5 — Leftovers from the first iteration

| Id | Item | Size | Depends | Acceptance |
| --- | --- | --- | --- | --- |
| N5-01 | Nebula patches near young clusters (M1-09/M10-14) | S | - | space frame in a nebula region |
| N5-02 | Sun pillars in cold air (M10-14) and visor glints (M1-03) | S | - | frames |
| N5-03 | Evolving gas giant storms (M9-10): a slow time term in the storm field | S | - | two globe frames a day apart differ |
| N5-04 | Run the web build with emsdk (KI-021) and fix what breaks; push the tree to GitHub so CI runs | M | - | a browser session; a green CI run |
| N5-05 | In-game key rebinding screen (the file stays) | M | - | settings page |
| | **N5 status 2026-09-26**: N5-01 done (`nebulaPatches`/`nebulaGlow` in `galaxy/starfield.*`: 3-6 patches per nebula cell of the arms, blue/red/white in the space backdrop, grey in the night sky; `space` renders `space_nebula.png`). N5-02 done (a sun pillar in cold clear air below 11 deg of sun altitude in `drawSky`; visor glints as two HUD arcs mirrored through the centre on the surface; `scene tundra_lowsun`). N5-03 done (the storm term kept in the gas giant map's veg channel, GEN_VERSION 4, and modulated by a day-long sine per latitude band in `drawGlobe`; `space` checks the frame a day later differs). N5-04 **blocked**: no emsdk here and the tree is not a git repository with a GitHub remote; needs the user (install emsdk, `web/build_web.sh`; `git init`, a remote, a push). N5-05 done (`KEYS` state from the settings' `KEY BINDINGS` item: 28 actions in two columns, Enter captures the next raw key, Backspace resets, Esc saves `vesperis_keys.txt`; `input` test drives it, `shots/keys.png`) | | | |

## N6 — Engineering health (continuous)

| Id | Item | Size | Depends | Acceptance |
| --- | --- | --- | --- | --- |
| N6-01 | Every milestone ships with `regress` frame hashes re-blessed on purpose, `bench check` green, the reference notes in `docs/reference/` updated in place, and a `PROGRESS.md` entry | ongoing | - | reviewed at each milestone |
| N6-02 | A `sheet` for look work: three or four scenes side by side before/after (kept in `shots/`, no code left behind, per the no-parallel-paths rule) | S | - | sheets in `shots/` |
