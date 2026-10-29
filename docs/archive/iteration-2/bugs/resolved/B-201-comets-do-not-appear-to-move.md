---
id: B-201
title: Comets do not appear to move
severity: S3
status: resolved 2026-09-26 (N0-01)
reported: 2026-09-26
---

## Where
State / screen: space (parked, approaching, or looking around the system); also the surface sky.
Star / body: any system with a `<star> comet I` in the analyzer (45% of systems since M5-07).

## Steps
1. Arrive at a system that lists a comet, target it (`Tab`, or `L` until it is the local target).
2. Watch it, parked or from a planet's orbit, at time warp x1 for a minute.

## Expected
A comet should visibly live: move against the stars, its tail streaming and changing as it swings round the star.

## Actual
Nothing changes. The coma and tail are drawn but look painted on the sky.

## Analysis (Claude)
The comet's position is a function of time (`StarSystem::bodyPos` with the Kepler solver, `docs/reference/04-galaxy-and-systems.md`), so it does move; the rates are just physical, and physical is slow:

| Situation (comet of `space_comet.png`: a = 5.05e6 km, e = 0.71, period 1.2 d) | Angular rate | On screen at x1 (1x, 70 deg FOV) |
| --- | --- | --- |
| at periapsis, watched from a planet 5e6 km away (speed about 740 km/s) | 0.0085 deg/s | 1 pixel every 25 s |
| half way out on its orbit, same observer | about 0.001 deg/s | 1 pixel every 3-4 min |
| parked at it (3.5 nucleus radii, 7-40 km away, co-moving) | 0 against the stars | nothing; the sun's direction turns 0.008 deg/s |

At x10 warp the drift becomes 0.4 px/s (visible within a few seconds); at x100 the tail swings round the star in a couple of minutes. Two more reasons it reads as frozen: the tail is a static glow with no internal motion, and the nucleus, a 2-12 km globe with a 3-30 h day, shows no rotation at x1 either. The analyzer's `ORBIT` column also shows the semi-major axis, not the current distance, so the numbers do not change while you watch.

## Fix (N0-01, size S-M)
* Animate the tail: knots of brightness that stream away from the coma at about 25 px/s (hashed from time, deterministic), a slight flicker of the coma, and a second, curved dust tail that trails along the orbit (the anti-sun ion tail stays straight). Motion without cheating on the orbit.
* Approach: when the local target is a comet, park at 60 nucleus radii instead of 3.5 so the coma, the tail's root and the streaming are in view, and show the speed (`km/s`) and `PERIAPSIS IN` / `RECEDING` on the HUD and the data sheet; the analyzer lists the current distance from the star for comets.
* Keep the orbital rates as they are (the sky must stay honest); mention in the help that time warp shows orbital motion.

## Acceptance
`vesperis_test space` renders the comet frame twice, 5 s apart at x1: the frame hashes differ, the printed knot positions moved, and at x100 (500 s apart) the comet's screen position moved by more than 4 px from the planet's orbit. A frame from the new parking distance goes to `shots/space_comet_close.png`.

## Outcome (2026-09-26, N0-01)
* `drawCometTail` now animates: the coma breathes (about 10%), five knots of brightness stream down the straight ion tail (`SpaceRenderer::cometKnot`, one traversal every 40 s, accelerating outward), and a second, dimmer dust tail curves back along the orbit (built from `StarSystem::bodyVel`, new). Close up (the nucleus over 3 px), two to four jets vent from the sunlit hemisphere and bend into the tail, knots every 8 s, turning with the nucleus.
* Parking at a comet is 12 nucleus radii (`Game::parkDistanceFor`, shared by the approach and the tests), not the 60 proposed: at 60 the nucleus is 8 px across at 2x, at 12 it is 76 px with the jets around it. The parked ship rides along, so from the seat the jets and the knots are the motion; against the stars the sun's direction still turns at its honest 0.008 deg/s.
* Readouts: the space HUD prints a second line under the local target for comets (`216 KM/S, PERIAPSIS IN 6.5 H` / `RECEDING, APOAPSIS IN ...`, from `StarSystem::meanAnomaly`); the data sheet adds `NOW <distance> FROM THE STAR` and the same motion line; the analyzer appends the speed in km/s to comet rows; the help says `T TIME WARP (ORBITS MOVE)`.
* Checks: `vesperis_test space` renders the comet frame twice 5 s apart (hashes differ, knot 0 moved from 0.170 to 0.295 of the tail), measures the drift seen from the first planet at x100 over 500 s (4.08 deg = 18.6 px at 1x, over the 4 px asked) and renders `shots/space_comet_close.png` from the parking distance (20,929 px change in 3 s). `vesperis_test flow` parks at the first comet near the start (`flow_comet`, `flow_comet_data`, `flow_comet_list`). The far frame is now taken from above the orbital plane so the dust tail's curve shows.
* The orbital rates are untouched (KI-202 closed as "explained and animated", not "sped up").
