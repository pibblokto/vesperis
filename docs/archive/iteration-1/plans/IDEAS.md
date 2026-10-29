# Ideas backlog

Unscheduled. Roughly ordered from cheap to ambitious inside each group.

## Small delights
* Capsule interior glow lighting the ground at night; footprints in sand and snow; dust puffs when landing.
* Screenshot sidecar text with location and time; photo mode that hides the HUD and lets the camera free-float.
* "Attract mode" on the title: slow fly-by of the home system.
* Star twinkle through atmospheres; shooting stars at night; satellites (the Stardrifter itself as a moving point seen from the surface, it is in orbit after all).
* Day-length and season readouts; a "time-lapse" key that warps time while keeping the camera still, then returns to x1.
* Name the home star and home world with a small ceremony on the first launch.
* Landing map: key auto-repeat while an arrow is held (after 0.3 s, 20 steps/s), click or drag on the map to place the cursor (from the withdrawn B-003; `Shift` + arrows already moves fast).

## Worlds
* Rivers, lakes, waterfalls and deltas on habitable worlds (flow field from the height function, carved consistently).
* Tidal-locked planets: a permanent twilight belt with its own climate, ice on the night side, deserts on the day side.
* Geysers on icy moons, sulphur plains on molten moons of gas giants, glass deserts on quartz worlds.
* Caves and overhangs (would need a 3D field or displaced meshes; big).
* Aurorae near the poles of worlds with atmospheres around active stars; visible pulsar beams sweeping the sky.
* Fog banks and low clouds you can walk into; dust storms that reduce visibility to metres; snowfall; hail.
* More materials: mud flats, salt pans, tundra, moss, coral reefs in shallow water.
* Flora families per planet (fibrous trees, transparent leaves, giant trees, umbrella forests, reeds) with seeded colour palettes.
* Fauna: herds, flyers that land, swimmers, night creatures; a "highlight animals" key.
* Ruins and monoliths with alien glyphs generated from the star name; the "Suricrasian cube" as an homage.

## Ship and interface
* Full Stardrifter interior and GOES console (roadmap M2).
* A working "planet finder" that reports how many planets and moons the targeted star has before you fly.
* Approach paths that circle the body before parking; a "high-speed orbit" mode that shows the whole day in minutes.
* A telescope view from the ship (zoom on the crosshair) to inspect distant bodies.
* Docking with a derelict Stardrifter found by chance (pure set-dressing).

## Astronomy
* Multiple-star systems with companion stars and two suns in the sky (roadmap M5-01).
* Substellar objects with glowing surfaces and moon systems (M5-02).
* Eccentric orbits and orbital resonances that make moon configurations recur.
* Rings seen from the surface as an arc across the sky (M5-04); ring shadows on the planet's clouds.
* Comets with tails near periastron; asteroid belts as sparkles between orbits.
* Real-time clock mode: the galaxy keeps moving while the game is closed (the original ran on the wall clock).

## Sharing (the GUIDE)
* Local catalogue with names and notes; export/import files to exchange with friends; "discoveries" counters.
* An "inbox" folder: drop someone's catalogue there and their names appear in your galaxy.
* Postcards: a PNG with the frame plus a strip of coordinates and time, ready to send.

## Presentation
* Optional 640x400 "hi-res" mode keeping the same palette and filter (the original's community mods did this).
* CRT shader (curvature, bloom, phosphor mask) in the platform layer, off by default.
* 4:3 aspect option (the original's pixels were not square).
* Gamepad support; key rebinding; mouse sensitivity and invert-Y settings.
* Web build via Emscripten (raylib supports it; the game core is already platform-free).

## Engineering
* Worker thread prefetching terrain caches ahead of the walking direction.
* Deterministic integer-based noise for cross-platform identical galaxies (only matters for sharing).
* Data tables (star classes, planet types, colours) in a text file for tweaking without recompiling.
* Frame-hash regression tests and a performance budget in CI.
