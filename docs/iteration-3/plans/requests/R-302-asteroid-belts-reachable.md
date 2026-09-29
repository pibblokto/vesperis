---
id: R-302
title: Asteroid belts the Stardrifter can fly to
status: done 2026-09-27 (O3)
requested: 2026-09-27 (verbal)
related: O3
---

## What
"I noticed system-level asteroid belts, but they are not approachable by the Stardrifter; it should be fixed, even though it's ok if we can't land on anything right now."

## My notes (Claude)
* A belt is a destination like a body: a row in the analyzer (`Tab`, after the bodies; `Enter` approaches, `L` targets), `L` in space cycles through the bodies and then the belts, `Enter` starts the fine approach, `X` centres the view on the rock, `I` opens the belt's sheet (edges, width, periods at both edges, a rock estimate, the rock you are parked at), `C` says there is no world under the ship.
* The rocks are a pure function of the system: the belt is cut into rings 400 km wide in radius, each ring turning at the Kepler rate of its middle radius (so the field shears as the inner rings outrun the outer, but a ring keeps its formation), into arc cells of 400 km and height cells; a cell hashes 1-3 rocks (fewer near the edges), radius `0.25 + 12 u^4` km (most under a kilometre) and one in two thousand a 35 km mountain, each with a seed, a tumble axis and a period of half an hour to two and a half. `StarSystem::beltRockAt(k, ir, ia, iy, m, t)` gives rock m of a cell at any time, so the rock the ship parked at is the same rock after a load.
* The approach keeps the ship's angle round the star, clamps its radius into the belt, picks the biggest rock within two cells of that anchor and parks six radii off it on the sunlit side (`Game::pickBeltRock`, `beltParkPos`); the parked ship drifts with the rock (mode `IN THE BELT`).
* Inside the belt the renderer draws the rocks within four cells (points under two pixels, lumpy meshes above: a sphere with fbm radii, 6 x 3 or 12 x 6 faces, back faces culled, lit by the star, depth-tested) in bank 3 (the rock ramp when no second world is in view; the cabin owns banks 8-10). From outside the belt stays the sparkle band. Cost 1.9 ms at 2x.
* Not done, as agreed: landing on a rock (the small-body surface code of O4 would carry it later).
