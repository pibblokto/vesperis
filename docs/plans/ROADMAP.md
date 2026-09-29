# Roadmap

**Status (2026-09-26): every milestone below (M0-M10) is implemented and documented; per-item notes sit in the last column of each table, open leftovers in `docs/bugs/KNOWN-ISSUES.md` and `IDEAS.md`. The section "Where we are" is the baseline the plan started from.**

## Where we are

A complete, playable loop: hashed galaxy with six sun classes, generated systems with moons and rings, Vimana flight, approach and orbit, landing map, descent, unbounded surfaces derived from the same planet function as the globes, real sky geometry (sun, moons, parent planets, stars), weather, simple life, HUD, menus, help, save/load, procedural audio, and a headless test harness that renders every scene. Frame time is about 1-2 ms at 320x200, so there is a large budget for richer worlds.

What is thin compared with the original and with what the engine could do:

* **Fidelity of the look**: foliage, water, lens flares, night lighting and airless contrast are simpler than Noctis IV's; only four palette banks per scene.
* **The ship as a place**: there is no cabin, no onboard computer screens, no GOES console. The Stardrifter is currently a camera.
* **The collector loop**: nothing is named, logged or catalogued. In the original this (the GUIDE) was the heart of the "no goal" game.
* **Surface variety**: no rivers, no ruins, few plant and animal forms, weather limited to rain and lightning, no jetpack, no vision modes, no photo tools.
* **Astronomy breadth**: single stars only; no multiple systems, substellar objects, eccentric orbits, rings in the sky, proper eclipses.
* **Presentation and platforms**: no settings menu, one save slot, macOS Makefile only, no gamepad, no web build.

## Where we can get

The end state I would aim for: a game that feels like Noctis IV remembered rather than emulated. You live in the Stardrifter, read its screens, type at the GOES console, name the stars you visit and keep a log with photographs; worlds have rivers, storms, ruins and creatures that belong to their star; two suns can rise on a moon under a ringed giant; and all of it stays deterministic and shareable. Everything below is sized so each item can be verified with a headless render or a scripted flow.

Suggested order: M0 (make iteration safe) -> M10 (the richer picture: resolution, textures, lighting; every later look decision should be made at the final resolution) -> M1 (the remaining look items) -> M8 (movement and the buggy, since they change every landing) -> M3 (the loop that gives exploration meaning) -> M9 (generation, in parallel with M4) -> M2 (the ship) -> M4 (worlds) -> M5 (astronomy) -> M6 (presentation) with M7 running alongside. M1, M8 and M3 can be interleaved with your bug reports.

Three milestones have their own detailed plans: `PLAN-movement-and-vehicles.md` (M8), `PLAN-generation.md` (M9) and `PLAN-look-hd.md` (M10, with prototype sheets in `shots/compare*.png`). The tables below for them are summaries; the plan files hold the designs.

---

## M0 — Safe iteration (bugs, tooling, settings)

Goal: fix what you report, and make every later change cheap to verify.

| Id | Item | Size | Depends | Acceptance |
| --- | --- | --- | --- | --- |
| M0-01 | Triage and fix your bug reports (`docs/bugs/`) | ongoing | - | each report reaches `fixed` with a test or screenshot. **B-001, B-002, B-004, B-005 fixed 2026-09-26** |
| M0-02 | Settings file `vesperis_settings.txt`: mouse sensitivity, invert Y, FOV, master volume, scanlines, window scale; a Settings screen in the menu | M | - | settings persist across runs; visible in the menu; headless test writes/reads them. **done 2026-09-26** (`vesperis_test settings`) |
| M0-03 | Determinism regression: hash the planet maps of ten fixed bodies and the height profile of three sites; `vesperis_test regress` fails on unintended change | M | - | test passes on rebuild; docs list how to bless a new baseline. **done 2026-09-26** (`tests/regress_baseline.txt`, 18 hashes: 5 systems, 10 maps, 3 site profiles) |
| M0-04 | Save slots (3) plus autosave every 5 minutes and on every landing/launch; versioned header; consistent state for saves made during descent/ascent | M | - | flow test exercises save in each state and reloads. **done 2026-09-26** (header v2 with `saved`, `name`, `phase`; slot picker screen; `loadNewest` for Continue) |
| M0-05 | CMake build in addition to the Makefile; GitHub Actions running `make vesperis_test && ./vesperis_test flow bench` on macOS and Linux | M | - | green CI badge; Linux build documented. **done 2026-09-26** (CMake verified locally with a pip-installed cmake, `ctest` green; the workflow file is in place but the tree is not a git repo yet, so no badge) |
| M0-06 | Split `game.cpp` (1100 lines) into `game_states.cpp`, `hud.cpp`, `autopilot.cpp`, `landing_map.cpp`; UI helpers (panels, lists) in `game/ui.*` | M | - | no behaviour change; flow frames identical (hash). **done 2026-09-26** (plus `persistence.cpp`; flow frames byte-identical) |
| M0-07 | Known-issue fixes: LOD skirts (KI-006), first-frame prefetch of the cache ring during descent (KI-009), rain only above the horizon (KI-017), HUD overlap near the top (KI-018), re-anchor the site beyond 50 km (KI-007) | M | - | issues closed in `KNOWN-ISSUES.md` with screenshots. **done 2026-09-26** |
| M0-08 | Headless "random walk" fuzz: 10,000 frames of random keys across states without crash or NaN | S | - | runs in CI. **done 2026-09-26** (`vesperis_test fuzz [N]`, `Game::testHealth`, in CI and ctest) |

## M1 — The look (fidelity pass)

Goal: frames that could be mistaken for the original's screenshots, while keeping the engine's advantages.

| Id | Item | Size | Depends | Acceptance |
| --- | --- | --- | --- | --- |
| M1-01 | Point-spray foliage ("greenmush"): canopies as hashed clusters of 1-3 px points whose count scales with screen size, trunk lines, distant trees as single dots; dense grass as short strokes near the camera; per-planet flora colours from `vegColor` (done together with M10-05) | M | M10-03 | `surface 3` scenes show soft, rounded canopies; no square quads visible. **done 2026-09-26** (canopy and bush blobs are point sprays, count from the on-screen radius, far blobs single dots) |
| M1-02 | Airless contrast: **delivered by M10-06** (terrain cast shadows) plus slope darkening and brighter rims | - | M10-06 | `surface 1` noon/sunset show readable craters. done 2026-09-26 with M10-06 |
| M1-03 | Lens flares and sun halo: rings and streaks along the line from the sun to the screen centre, dimmed by atmosphere and rain; visor glints; corona rays for blue giants | S | - | `sun_S0N` and `surf_*_sunset` show flares; flares disappear when the sun is occluded. **done 2026-09-26** (`drawLensFlare`: four discs and a streak, gated by haze, rain, eclipse and occlusion; blue giants get six rays; no visor glints) |
| M1-04 | Water: mirrored terrain reflection (second terrain pass with inverted heights and half vertical resolution, like the original's `halfscan`), animated incoming wave rings from the wind direction, foam sparkle at the shoreline, refraction-like grain wobble | L | - | `surface 3` ocean scene shows reflections and waves; frame time under 4 ms. **done 2026-09-26** (`drawReflections`: land mirrored about sea level blended into water pixels only, far to near; wind-driven wave rings; shore foam sparkle; the water grain already drifts; `scene felisian_shore`) |
| M1-05 | Night lighting: moonlight from the brightest body above the horizon (colour and strength from its phase and size), starlight ambient, the capsule beacon lighting the ground around it, darker shadows | M | M1-02 | `surface *_night` frames show terrain shapes under a bright moon and near-black elsewhere. **done 2026-09-26 with M10-09** |
| M1-06 | Colour capacity experiment: prototype 8 banks x 32 shades (or 6 x 42) with error-diffusion on the intensity; compare A/B frames; if accepted, give felisian globes land and ocean banks, keep coloured stars inside systems, and free a bank for objects | L | - | decision recorded in `10-decisions.md`; frames side by side in `shots/`. **superseded by M10-03** (16 banks x 12 bits): felisian globes now use a land and an ocean bank (2/6, 3/7), stars stay coloured, snow on felisian surfaces has bank 8 |
| M1-07 | Atmosphere: horizon haze band and slight curvature drop of the far ring, aerial perspective per material (distant greens go blue), venusian blur haze (`psmooth` of the lower screen), exotic twilight colours on thin-atmosphere worlds from `skyTint` | M | - | `surface 2/5` scenes visibly hazy; horizon line no longer razor sharp. **done 2026-09-26** (haze band, curvature drop of terrain and water, venusian third mush pass, thin-atmosphere twilight from `skyTint`; aerial perspective stays the fog toward the horizon colour) |
| M1-08 | Eclipses and shadows: ring shadow on the planet, moon shadows on the planet globe, partial and total solar eclipses from a moon's surface with sky darkening and a corona | M | - | `moonsky` test extended with an eclipse time found by search; frames show the shadow. **done 2026-09-26** (globe pixels shadowed by moons, the parent and the rings; `SunInfo::eclipse` coverage dims the light and the sky; the sun is depth-tested so bodies carve it; corona grows with coverage; `moonsky` writes `shots/eclipse.png`) |
| M1-09 | Galactic backdrop: a faint band of unresolved stars from the density function (the galaxy seen edge-on from inside), coloured nebula patches near young clusters, glitter of distant stars during Vimana | M | - | space frames show the band; no visible tiling. **done 2026-09-26** (`buildGalaxyBand`/`sampleGalaxyBand` shared with the surface sky; no nebula patches) |
| M1-10 | Vimana and approach feel: stronger streaks with colour shift, arrival flash and settle, orbit motion cues (the body slowly rotating in view), "field amplificator" toggle showing more stars (the targeting part ships earlier with bug B-005) | S | - | `flow_vimana.png` and `flow_arrived.png` visibly changed; toggle in help. **done 2026-09-26** (blue-shifted streaks ahead, red behind; arrival flash; `F` toggles the field amplificator) |
| M1-11 | CRT option in the platform layer (curvature, bloom, mask) via a raylib shader, off by default | S | M0-02 | setting persists; screenshot unaffected (taken before the shader). **done 2026-09-26 with M10-17** |

## M2 — The Stardrifter (ship interior and onboard computer)

Goal: the ship becomes a place you inhabit, like the original.

| Id | Item | Size | Depends | Acceptance |
| --- | --- | --- | --- | --- |
| M2-01 | Walkable cabin: 5x5x1 m box with windows on all sides and the roof, first-person walking inside, collision with walls and consoles, the light bulb, the fuel orb (decorative, slowly pulsing), the capsule net in the middle | L | - | you can walk to any window and look out; the space view is drawn through the windows with correct parallax. **done 2026-09-26** (`game/cabin.cpp`; 5x5x2.4 m; the classic cockpit camera stays as the SHIP VIEW setting) |
| M2-02 | Front screen computer: Flight control / Onboard devices / Preferences pages navigated with mouse clicks and keys, mirroring today's key commands; auto-screen-off | M | M2-01 | every key command is reachable from the screen. **done 2026-09-26** (keys only, no mouse clicks; no auto-off) |
| M2-03 | GOES console: a text terminal on the left wall with commands `SL [range]` (list stars), `DL [name]` (list bodies), `PAR name` (coordinates), `ST name` (set target), `WHERE name`, `CAST text` (note), `CAT` (read notes), `HELP` | L | M3-02 | commands work on generated and renamed objects; output scrolls; typed input with the small font. **done 2026-09-26** (plus `HOME`, `PREV`) |
| M2-04 | Roof observation deck reached by the lifter, depolarise (transparent hull), internal light toggle with ramp, cabin hum tied to the light | M | M2-01 | `flow` test extended with a roof visit screenshot. **done 2026-09-26** (`PgUp`/`PgDn`, `Y`, `U`; the hum is unchanged) |
| M2-05 | Capsule as an object: you enter it in the cabin, it drops through the floor, on the surface you exit through a door; returning means climbing in | M | M2-01 | descent/ascent start and end inside the capsule. **done 2026-09-26** (the capsule sits in the cage; `E` at the cage deploys it; no drop animation) |
| M2-06 | Right-wall screens: the landing map on the middle screen, environment data on the right, target data on the left (today's overlays become screens) | M | M2-02 | overlays still available by key for speed. **done 2026-09-26** (left: star map, middle: landing map, right: target data) |

## M3 — Cartography and the GUIDE (the collector loop)

Goal: give the no-goal game its memory: names, logs, photographs, statistics, sharing.

| Id | Item | Size | Depends | Acceptance |
| --- | --- | --- | --- | --- |
| M3-01 | Star map screen: 3D view of the neighbourhood (rotate with the mouse, zoom, class colours and sizes, visited markers, current position, remote target); click to set the remote target; filter by class | L | - | headless frame of the map; targeting from the map works in `flow`. **done 2026-09-26** (`M` in space; rings at 2/5/10 LY, class filter `1`-`6`, the star nearest the crosshair is described and `Enter` targets it) |
| M3-02 | Naming: rename stars and bodies (text entry with the small font); stored in `guide.txt` keyed by star sector and body index; names used everywhere (HUD, list, console) | M | - | rename in `flow`, reload, name persists. **done 2026-09-26** (guide menu `G`; a renamed star or planet renames the generated names of its planets and moons too) |
| M3-03 | Expedition log: automatic entries on arrival, orbit and landing (EPOC, coordinates, class, conditions), free notes at any time, a log screen with paging | M | M3-02 | log screen frame in `flow`; file readable in a text editor. **done 2026-09-26** (`J`; arrival, orbit, landing, launch and note entries) |
| M3-04 | Gallery: screenshots get a sidecar `.txt` with place and time; in-game gallery browser showing the 320x200 PNGs with captions | M | - | gallery frame in `flow` after two screenshots. **done 2026-09-26** (`readPNG` for the game's own files; screenshots numbered after the existing ones) |
| M3-05 | Discovery statistics: systems visited, worlds landed, classes and types seen, furthest distance from home, longest walk, highest point; a statistics screen | S | M3-03 | numbers update in `flow`. **done 2026-09-26** (plus distance walked and driven, names given, photographs) |
| M3-06 | Target by name or coordinates (typed), travel history with "return to previous star", home star bookmark | S | M3-02 | works from the console (M2-03) and from a key. **done 2026-09-26** (guide menu entries; the console of M2-03 will reuse `targetStarByName`) |
| M3-07 | Sharing: export the guide (names + notes + log) to a single file; import a friend's guide into an "inbox" so their names appear in your galaxy (their names shown in a different colour) | M | M3-02 | round trip in a test; conflicts handled (yours win). **done 2026-09-26** (`guide_export.txt`; `guide_inbox.txt` imported into the inbox, shown in cyan; your names win) |

## M4 — Worlds with more life

Goal: every landing has something to find.

**Status (2026-09-26): complete.** M4-01 and M4-03 were moved to M8 and M9 and are done there; see `docs/reference/06-surface.md` ("Life", "Ruins", "Weather") and `07-game-flow-and-controls.md`.

| Id | Item | Size | Depends | Acceptance |
| --- | --- | --- | --- | --- |
| M4-01 | Jetpack, speed presets, postures: **moved to M8-03, M8-04, M8-11** | - | - | see `PLAN-movement-and-vehicles.md` |
| M4-02 | Sector map (F10): minimap of the cached terrain around you (heights as shades, water, capsule, trail of where you walked), zoom levels | M | - | done 2026-09-26 (`N` on foot; 2/8/32 km zooms, heights as shades, water, capsule, trail, waypoint placement with `M`; `flow` frame `sectormap`) |
| M4-03 | Hydrology: **moved to M9-04** (rivers, lakes, deltas) | - | - | see `PLAN-generation.md` |
| M4-04 | Flora families: 5-6 tree archetypes (broadleaf spray, conifer cone, fibrous stalk, umbrella, giant), transparent-leaf variants, cacti and succulents on deserts, snow flora, quartz crystal "trees"; one or two families per planet chosen by seed; sizes scaled by gravity | L | M1-01 | done 2026-09-26 (five families hashed per body, two per world, conifers forced in taiga/tundra, cacti on desert sand, cushion flora on tundra; sizes scale with gravity; no transparent-leaf variant, quartz crystals stay the M1 spikes) |
| M4-05 | Fauna: herds of grazers with flocking, reptile-like crawlers, flyers that land and take off, night creatures with eye glints, swimmers near shores; behaviours (flee when approached, graze, wander); a highlight key | L | - | done 2026-09-26 (1-3 herds of grazers and crawlers that keep together, flee within 12 m, flocks that land to rest, night eye glints, fins in deep water; `X` highlights them; scene `felisian_herd`) |
| M4-06 | Weather system: snow on cold worlds, dust storms on thin-atmosphere worlds (visibility to metres, howling wind), fog banks, hail; per-site climate from map moisture and latitude; aurorae near the poles of worlds around active stars; meteors at night | L | - | done 2026-09-26 (snow, dust storms with tinted palette, fog banks, hail, aurorae at high latitude around active stars; meteors came with M10; `surface` night and north frames) |
| M4-07 | Ruins and monoliths: rare procedural structures (columns, cubes, domes, walls) on habitable and quartz worlds, three texture styles, placed by hash so they are findable again; glyphs from the star name; a rare giant "cube" homage | L | - | done 2026-09-26 (5% of 2 km latitude/longitude cells on felisian, quartz and ocean worlds: columns, cube, dome, walls, a rare 40 m cube; smooth, striated or glowing-edge styles; glyphs of the star name; colliders; scene `felisian_ruin`) |
| M4-08 | Molten worlds alive: lava flow animation along cracks, glow lighting the surrounding rock at night, heat shimmer, occasional eruptions with lit plumes, lava sound | M | - | done 2026-09-26 (flowing cracks, night glow lighting, shimmer, eruptions with plumes and rumble; `surface 0` night frame) |
| M4-09 | Sound per world: wind families (sea, desert, mountain), surf on shores, lava rumble, rain intensity, bird calls, footsteps by material, capsule hiss; ship interior hum by state | M | - | done 2026-09-26 (wind tone by place: sea, desert, mountain whistle; surf near water; bird calls by day; footsteps by material with a splash when swimming; lava rumble and rain kept; `vesperis_test audio` writes `shots/audio_test.wav` and checks for NaN and clipping) |
| M4-10 | Vision modes: radiation visor (grainy but clearer at night), supervision (blue scale), infrared (heat from lava and creatures), plant vision; implemented as palette remaps | S | - | done 2026-09-26 (`V` on foot cycles radiation visor, supervision, infrared, plant vision as palette remaps; `flow` frame `supervision`) |

## M5 — Astronomy extensions

Goal: skies that surprise.

**Status (2026-09-26): complete** (generation version 3; see `docs/reference/04-galaxy-and-systems.md` "Generation version 3" and `06-surface.md` "Two suns").

| Id | Item | Size | Depends | Acceptance |
| --- | --- | --- | --- | --- |
| M5-01 | Multiple-star systems: a seventh class "multiple" whose companion stars are bodies of a new "companion star" type with their own planets; two suns in the sky with mixed light colour; double shadows in the lighting model | L | - | done 2026-09-26 (19% of systems: a `PT_COMPANION` body with its own class, 35% close binaries inside the first orbit, wide ones with up to three worlds of their own; the class reads `S00+S01 MULTIPLE`; surfaces blend both lights and draw two discs; blob shadows double; `space` frame `space_binary`, `moonsky` frame `binary_surface`) |
| M5-02 | Substellar objects: very large glowing bodies (brown-dwarf planets) with moon systems, warm night skies on their moons | M | M5-01 | done 2026-09-26 (`PT_SUBSTELLAR` in the cold band with luminosity 0.0008-0.005, banded, self-glowing in space and in a moon's sky (bank 13), warming its moons; frame `moonsky_substellar`) |
| M5-03 | Eccentric orbits (Kepler equation), visible motion of close moons in the sky within minutes, mutual occlusions between moons | M | - | done 2026-09-26 (Kepler orbits: `ecc`/`argPeri` per body, Newton solver in `bodyPos`; planets mostly round, captured moons 0.1-0.4, comets 0.6-0.93; frame `moonsky_0_later` 15 minutes on; globes depth-test each other so moons occult) |
| M5-04 | Rings from the surface: the ring plane rendered as an arc across the sky from ringed planets and their moons, with the planet's shadow on it at night | M | M1-08 | done 2026-09-26 (the ring plane intersected per sky pixel from the surface of a ringed world, with the world's own shadow at night; from a moon the parent's rings come with the globe; frames `rings_surface_day/night`, `moonsky_rings`) |
| M5-05 | Real-time clock mode (game time = wall clock since a fixed epoch, like the original), selectable at new game; EPOC/triad display fidelity | S | M0-02 | done 2026-09-26 (setting CLOCK: game time or the real UTC clock since 2026-01-01; the data sheet shows EPOC:SINISTER.MEDIUS.DEXTER; warp refused in real time) |
| M5-06 | Seasons: solstice/equinox readout, day-length curve, polar day and night, a time-lapse key (auto warp with a still camera, returns to x1) | S | - | done 2026-09-26 (data sheet: declination, day length or polar day/night, hours to the next equinox and solstice; `Ctrl+T` time-lapse x600 for 25 s; `vesperis_test seasons` compares 82 N at both solstices) |
| M5-07 | Asteroid belts (sparkle bands between orbits, targetable as "belt") and comets with tails near the star | M | - | done 2026-09-26 (belts in M9-12; comets as `PT_COMET` bodies on oval tilted orbits, coma and a kilometres-wide tail away from the star, brighter near periapsis; frame `space_comet`) |
| M5-08 | Tidal-locked worlds: permanent day/night climates in the planet function (ice on the night side, deserts under the sun, a twilight belt with life) | M | - | done 2026-09-26 (`Body::locked` planets carry the sub-stellar direction; felisian climate warms with it and freezes the far side, deserts under the sun, ocean worlds freeze their night side; frame `map_LOCKED_FELISIAN`) |
| M5-09 | Bodies in the sky more often (request R-001): bright wandering-star points for far bodies, daytime discs through the sky, landing map "in the sky" readout and jump key, double planets, bigger first moons, tighter gas giant moon systems; frequency targets measured by a `survey` test mode | M+M | B-005, M9-01 | done 2026-09-26 (points survive the mush as scale+1 squares, brighter by phase; landing map `IN THE SKY` line and `J` jumps under the biggest body; data sheet lists what is up; double planets 7%, first moons bigger and closer, giants' inner moons tighter; survey: >1 deg 39% (target 45-50), >3 deg 33% (35), >10 deg 18% (12-15), sister moons 8%: left there so it stays a treat) |

## M6 — Presentation, platforms, accessibility

**Status (2026-09-26): complete**, the web build written but unverified (KI-021).

| Id | Item | Size | Depends | Acceptance |
| --- | --- | --- | --- | --- |
| M6-01 | Settings screen extended: resolution scale (320x200 or 640x400 with the same palette and filter), 4:3 aspect option, palette dithering, HUD colour | M | M0-02 | done 2026-09-26 (render scale came with M10-01; added ASPECT square pixels or the original 4:3 with 1.2x tall pixels in the present layer, DITHER (ordered 2x2 over the 64 stops in `toRGB`), HUD COLOUR green/amber/white via `setHudScheme`; 18 items fit at an 8 px pitch) |
| M6-02 | Key rebinding and gamepad support (raylib gamepad API in the platform layer) | M | M0-02 | done 2026-09-26 (`vesperis_keys.txt` physical-to-logical remap applied in the front end, `vesperis_keys.example.txt` for AZERTY; gamepad in the platform layer mapped in `core/input.cpp`: left stick analogue walking/driving, right stick look, A enter, B/start escape, X jump, Y data sheet, bumpers sprint/chase view, triggers crouch/sprint, d-pad arrows, back analyzer; `vesperis_test input` checks both without a window) |
| M6-03 | Linux and Windows builds (CMake, raylib from the package or fetched), packaging (macOS app bundle, zip with the binary) | M | M0-05 | done 2026-09-26 (CMake fetches raylib 5.5 when none is installed, MSVC flags, CPack zip, `install` rules; `packaging/make_app.sh` builds `dist/Vesperis.app` with the raylib dylib inside, `packaging/make_zip.sh` a portable zip; CI: headless on Linux/macOS, `package` job on Linux and Windows uploading zips; the CMake tree builds locally with warnings as errors in core and galaxy) |
| M6-04 | Web build with Emscripten (raylib supports it; the core is platform-free) | L | M0-05 | written 2026-09-26, untested here (no emsdk on this machine): `main_raylib.cpp` runs one `step` per browser frame through `emscripten_set_main_loop_arg`, `web/shell.html` mounts IDBFS as the working directory so saves persist, `web/build_web.sh` drives `emcmake` with `PLATFORM=Web`; KI-021) |
| M6-05 | Photo tools: photo mode (HUD off, free camera), panoramic snapshot (three views stitched), motion picture recorder (frame sequence to `movies/`) | M | - | done 2026-09-26 (`Ctrl+P` photo mode: HUD off, on the surface a free camera flying with W A S D, Space/C and Shift; `Ctrl+Shift+P` a 3x panorama `shots/panorama_N.png`; `Ctrl+R` records every frame to `movies/take_NNN/`; `flow` frames `photo`, `photo_flight`, `flow_panorama.png`, three recorded frames) |
| M6-06 | Title and attract mode: slow fly-by of the home system, credits screen with the homage text | S | - | done 2026-09-26 (after 20 s idle on the title the camera tours the system, 14 s per body at four radii; `C` opens the credits page; `flow` frames `attract`, `credits`) |

## M8 — Movement and vehicles (see `PLAN-movement-and-vehicles.md`)

Goal: walking with weight and feedback, a real sprint, a jetpack, and a small buggy that makes 5-50 km trips fun.

| Id | Item | Size | Depends |
| --- | --- | --- | --- |
| M8-01 | Walking feel: head bob, landing dip, sliding on steep slopes, step-up, air control | M | - | done 2026-09-26 |
| M8-02 | Sprint as a mode: hold or toggle, stamina, FOV kick, breath and pulse readout, gravity-scaled speed | S | M0-02 | done 2026-09-26 |
| M8-03 | Jetpack: thrust, translation, ceiling, heat limit, altitude readout | M | M8-01 | done 2026-09-26 |
| M8-04 | Autowalk and speed presets 1-9, 0 stops | S | - | done 2026-09-26 |
| M8-05 | Collision with large rocks and trees (cylinders), stepping over small rocks | M | - | done 2026-09-26 |
| M8-06 | The buggy: deployment from the capsule, arcade driving on the terrain, dashboard and chase views, dust and tracks, headlights, engine sound, HUD, persistence | XL | M8-05 | done 2026-09-26 |
| M8-07 | Long-range trips: site re-anchoring, waypoints, capsule bearing line, buggy never lost | M | M8-06, M4-02 | done 2026-09-26 (waypoint by key `M`; the sector map of M4-02 will add map placement) |
| M8-08 | Water: wading by depth, diving, underwater palette, wave bob | M | M8-02 | done 2026-09-26 (no wave bob) |
| M8-09 | Gravity tuning of jumps, air control and falls | S | M8-01 | done 2026-09-26 |
| M8-10 | Control settings and gamepad analogue movement | S | M0-02, M6-02 | settings done 2026-09-26 (sprint hold/toggle, sprint speed); gamepad waits for M6-02 |
| M8-11 | Postures: crouch, "stand on hind legs" | S | - | done 2026-09-26 (`C` crouch, `Z` held stands tall) |

## M9 — Generation improvements (see `PLAN-generation.md`)

Goal: worlds with geological structure, hydrology, biomes and seasons, tuned with data, still one deterministic function.

| Id | Item | Size | Depends |
| --- | --- | --- | --- |
| M9-01 | Survey and gallery tools for data-driven tuning | S | - | done 2026-09-26 (`survey`, `gallery <type>`) |
| M9-02 | Tectonic plates: mountain chains, rifts, island arcs, plate-shaped continents | L | M9-01 | done 2026-09-26 |
| M9-03 | Erosion look: slope-dependent detail, valleys, talus, mesas and terraces | M | M9-02 | done 2026-09-26 (no talus fans) |
| M9-04 | Rivers, lakes and deltas from a baked flow map | XL | M9-02 | done 2026-09-26 as a hashed network with local water levels (see the plan's status note) |
| M9-05 | Biomes from temperature and moisture with rain shadows | L | M9-04 | done 2026-09-26 |
| M9-06 | Craters with history: power law, rays, chains, basins, mare flooding | M | - | done 2026-09-26 (no chains) |
| M9-07 | Volcanism on every suitable type, cryovolcanoes and geysers | M | M9-02 | done 2026-09-26 (no hotspot chains) |
| M9-08 | Dune seas from a wind field | M | M9-05 | done 2026-09-26 |
| M9-09 | Ice and seasons: caps, sea ice, glaciers, moving snow line | M | M9-05 | done 2026-09-26 (caps, sea ice, snow line, frozen lakes; glaciers only as ice above the snow line) |
| M9-10 | Living gas giants: differential rotation, evolving storms, night lightning | M | - | done 2026-09-26 except evolving storms (the map is static; jets and lightning move) |
| M9-11 | New body types: ocean, metal, tidally heated volcanic moon, carbon world | L | M9-01 | done 2026-09-26 |
| M9-12 | System architecture: belts, resonances, captured moons, ring gaps | M | - | done 2026-09-26 (eccentric orbits stay with M5-03) |
| M9-13 | Galaxy regions with distinct populations and name styles | M | - | done 2026-09-26 (no map colouring yet) |
| M9-14 | Generated descriptions for bodies and stars | S | - | done 2026-09-26 |
| M9-15 | 512x256 maps with relief shading, background generation | M | M7-01 | done 2026-09-26 (worker thread per map; globes draw flat until ready) |
| M9-16 | Distribution tuning: more temperate worlds, moon variety | S | M9-01 | done 2026-09-26 (felisian 9% of bodies, in 54% of systems near the start) |
| M9-17 | Generation versioning in saves and guide files | S | - | done 2026-09-26 (`GEN_VERSION` 2) |

## M10 — A richer picture, same vibe (see `PLAN-look-hd.md`)

Goal: more definition (up to 4x, default probably 2x with a 3x3 mush), generated per-material textures, real lighting (cast shadows, sky light, secondary lights, glints), a richer sky, all through palette banks and the mush so the character stays. Prototype sheets for the resolution part are in `shots/compare*.png`.

| Id | Item | Size | Depends |
| --- | --- | --- | --- |
| M10-01 | Runtime resolution scale 1x-4x in the settings | M | M0-02 | done 2026-09-26 |
| M10-02 | Mush kernel per scale, softness setting, 3x/4x sheets and default decision | S | M10-01 | done 2026-09-26 (default 2x) |
| M10-03 | 16-bit pixels: 16 banks, 12-bit intensity, interpolated palette | L | - | done 2026-09-26 |
| M10-04 | HUD and font at scale | M | M10-01 | done 2026-09-26 |
| M10-05 | Per-material texture stack (macro/meso/micro), object textures, decals | L | M10-03 | done 2026-09-26 |
| M10-06 | Terrain cast shadows with soft edges | M | - | done 2026-09-26 |
| M10-07 | Sky light and coloured sun on lit faces | S | M10-03 | done 2026-09-26 |
| M10-08 | Cost work for 3x/4x (half-res sky, threaded rows) | M | M10-01 | done 2026-09-26 |
| M10-09 | Secondary lights: lava glow, planetshine, beacon pool | M | M10-06 | done 2026-09-26 |
| M10-10 | Speculars, glints and sparkle on water, ice, snow, quartz, lava | S | M10-05 | done 2026-09-26 |
| M10-11 | Object blob shadows | S | M10-06 | done 2026-09-26 |
| M10-12 | Quantised gradient shading mode next to flat | S | M10-03 | done 2026-09-26 |
| M10-13 | Scattering lite: view-dependent fog, haze, sunset tint | M | M10-03 | done 2026-09-26 |
| M10-14 | Sky upgrades: two cloud layers, halos, twinkle, galactic band, meteors | L | M10-08 | mostly done 2026-09-26 (no sun pillars, no nebula patches) |
| M10-15 | Grain, lines, points and strokes in scale units | S | M10-01 | done 2026-09-26 |
| M10-16 | Upscale filter and scanlines at fractional scales | S | M10-01 | done 2026-09-26 |
| M10-17 | Optional bloom and CRT, off by default | S | M6-01 | done 2026-09-26 |
| M10-18 | Decision record, reference updates, before/after sheets per item | S | - | done 2026-09-26 |

Absorbs M1-01 (foliage spray, via M10-05) and M1-02 (airless contrast, via M10-06).

## M7 — Engineering health (continuous)

**Status (2026-09-26): all items done**; M7-05 continues with every change.

| Id | Item | Size | Depends | Acceptance |
| --- | --- | --- | --- | --- |
| M7-01 | Worker thread prefetching the terrain ring ahead of the walking direction; descent fills lod2/lod1 first | M | - | done 2026-09-26 (`SurfaceView::prefetchAhead`: a worker thread samples the uncached cells 24-46 cells ahead within 55 deg of the heading, alternating lod0 and lod1, up to 320 cells per job, copied into the caches on the next frame; the descent keeps warming lod2/lod1/lod0; bench: landing first frame 1.4 ms, sprint worst frame 1.6 ms, 400 cells delivered) |
| M7-02 | Map cache policy (LRU with a size cap), generation profiling counters printed by `bench` | S | - | done 2026-09-26 (least-recently-used eviction at `mapCacheCap` 24 maps (about 19 MB), never a map mid-generation; counters generated/hits/evicted and the worker generation time (113 ms per 512x256 map) printed by `bench`; unit test with a cap of 3) |
| M7-03 | Unit tests: frame math (lat/lon round trips, sun altitude against analytic cases such as equator at noon = 90 - declination), noise statistics, name generator length bounds | M | - | done 2026-09-26 (`vesperis_test unit`: lat/lon and site frame round trips, the sun at the zenith over the sub-solar point and at 60 deg 30 deg along the meridian, local noon, Kepler residuals, noise mean/bounds/determinism, hash uniformity, name lengths and alphabet, seasonOf bounds, map cache cap; in ctest and CI) |
| M7-04 | Frame-hash regression for `flow` and `surface` scenes with a documented bless procedure; performance budget assertions | M | M0-03 | done 2026-09-26 (`regress` gained four frame hashes at 1x: scenes felisian_shore, cratered_noon, molten_night and a space frame from the first orbit of a yellow star, maps warmed first so they are exact; `bench check` fails on budgets space 2x 12 ms, surface 2x 16 ms, descent first 60 ms, sprint worst 40 ms; CI runs both) |
| M7-05 | Documentation upkeep: reference docs updated with every milestone; decision log entries for every trade-off | ongoing | - | ongoing; every milestone updated the reference notes, decisions and known issues on 2026-09-26 |
| M7-06 | Code style: `.clang-format`, warnings as errors for `core` and `galaxy` | S | - | done 2026-09-26 (`.clang-format` with the house style; `-Werror` on `src/core` and `src/galaxy` in CMake (`VESPERIS_WERROR`), the tree builds clean) |

---

## Quick wins (each under two hours, independent)

* M1-03 lens flares; M1-10 Vimana feel; M3-05 statistics; M3-06 target by name; M4-10 vision modes; M5-05 real-time clock; M5-06 season readouts; M6-06 attract mode; M7-06 style; M8-02 sprint mode; M8-04 autowalk; M8-11 postures; M9-01 survey tool; M9-14 descriptions; M9-16 distribution tuning.
* Small polish not listed above: capsule beacon lights the ground at night; birds land at dusk; screenshot sidecar text; a "you are here" marker in the system list; sort the system list by distance.

## Risks and open questions

* **Palette capacity** (M1-06) is the one structural decision left. Four banks are faithful; eight would allow coloured stars and two-tone planets but change the texture of the image. I would prototype both and choose by screenshots.
* **Rivers** (M4-03) are the hardest generation feature: they must be a function of position with no global simulation. A workable approach is hashed "river seeds" per continent with downhill tracing baked into a coarse map that the fine function samples; consistent, but a few days of work.
* **The cabin** (M2) changes the space state from a camera to a small 3D scene; the rasteriser already supports it, but window geometry, walking collision and screens are a chunk of UI work.
* **Determinism across platforms** only matters once guides are shared (M3-07); an integer-noise variant would solve it at some cost in speed.
