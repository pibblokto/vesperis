# B-506: frames drawn before their maps were made

status: resolved 2026-11-23 (J-16)
severity: S4
area: the harness's scenes (`harness/test_game.cpp`: `TestGame::testWaitMaps`; `modes_rogues.cpp`, `modes_probe.cpp`, and
`modes_kinds.cpp`'s `qualityFrame`)

## Report

KI-446, found in R-438 (2026-11-19): "A battery item differs from its snapshot by a moving frame, not run `twice`: `rogues` (the radar
lock's status band: y03 and y02 each had one of its two states), `sweeps` (`trait_fog_seas`), `probe` (`probe_02_entry`, once)."

## Cause

A body's globe is drawn from its map, which is made on a thread of its own: `SpaceRenderer::mapIfReady` asks for it, and until it is
made the globe is drawn flat (its colour and its light, no ground, no bands). A frame shot soon after a scene reached a new place drew
each globe whole or flat by how fast that thread had run beside the scene's frames, and the machine's load decides that:

- **`rogues heard`** waited for the home system's maps before its first frame, but before any frame had set the season. The first
  frame's `setSeason` made two of them again (a world's season step is kept by its seed: B-438), and the radar's lock frame, nine
  seconds on, drew the home world's limb under the readout whole in four snapshots (r438, y01, y03, y04) and flat in the others. At
  the drifters it flew to and parked at, it waited for nothing.
- **`probe wav`** waited for nothing: the giant falls under the probe for 36 s before its entry, and its first three pictures drew it
  whole or flat.
- **`traits`** draws its quality sheets through a renderer of its own, and the sky of a surface frame never waits for a map: a
  moon in the fog seas' valley view was drawn whole in most runs and flat in R-438's and B-443's. The first frame of a sheet whose
  moons were not yet asked for drew them flat every time.

The old binary, eight runs of each scene, four at a time (a machine as busy as a battery's beside other work):

- `rogues heard` drew two of its ten pictures two ways (`rogue_disc_orbit` six whole and two flat, `rogue_giant_orbit` seven and
  one), and its lock frame whole in all eight, where j15's is flat. Three runs more drew `rogue_disc_60` whole once.
- `probe wav` drew `probe_01_fall` four whole and four flat, and `probe_02_entry` whole in all eight, where j15's is flat.
- `traits fog_seas` alone drew its morning view's moon flat in six runs of six: its first frame is the one that asks for the map.

## Fix

- **`TestGame::testWaitMaps`**: every body's map of the system, made for the season the next frame draws (set first, so the frame
  does not make it again), at one instant between frames: no clock moves, and the cache is held large enough for all of them.
- **`rogues`**: `heard` waits so at its start (in place of its wait before the season), after the flight to the drifter and at each
  drifter it turns to or parks at; `dark` and the plain mode after each parking.
- **`probe`**: before each picture's frame.
- **`traits`**: `qualityFrame` makes the sky's other bodies' maps before its frame (`mapFor`), at the season the renderer holds for
  them (the sky sets none).

The pictures now show every body with its map. Those that drew one flat in the snapshots change once: 7 of `rogues`' 16 (the
radar's lock, the arrival, the black disc and the giant from 60 radii and parked, the brown dwarf's moon) and two of its printed lines
(the discs' middle rows); 3 of `probe`'s 15 (the fall's two and the entry); 3 of `sweeps`' 505 (`trait_great_aurorae`, its ringed
parent, 2,463 px; `trait_fog_seas`, its morning view's moon, 709 px; `trait_airglow`, 7 px). `rogues dark`'s six pictures do not move.

The rule (`rules-harness.md`, "Frames that repeat"): at each new place a scene waits for every map of the system, made for the season
its frames draw.

## Verification (2026-11-23)

- **The new binary, eight runs of each, four at a time:** `rogues heard`'s ten pictures and `probe wav`'s fifteen each came out one
  way in all eight; `traits fog_seas` one way in six of six; the whole `traits` twice, its 47 sheets the same in both.
- **Old against new:** `rogues dark`, four runs each, its six pictures j15's on both. The whole `traits`: 44 of its 47 sheets the same,
  the three above moved. The moved pictures show the body's ground or bands where the old drew a flat disc (`rogue_bd_moon`'s craters,
  `probe_00_fall_start`'s bands, `trait_great_aurorae`'s parent).
- `make check`. KI-NEW-C (the parent gives its id): other scenes that shoot soon after reaching a place still do not wait.
