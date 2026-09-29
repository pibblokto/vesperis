---
id: B-308
title: Space turns yellow when the camera swings past a companion star; the sun explodes near the edge of the view
severity: S2
status: resolved 2026-09-27
reported: 2026-09-27
---

## Where
State / screen: space, in orbit of a companion star (also near a comet close to it)
Star / body shown in the HUD: CRIPHAIN B (companion star), PARSIS +174 +7 +97, `IN ORBIT: CRIPHAIN B - COMPANION STAR - NOT LANDABLE`, LOCAL 1.94 MKM
EPOC shown in the HUD: 6011:003.777
Save attached: no (four screenshots in the report)

## Steps
1. Approach a companion star (the ship parks 3.5 radii off, `Game::parkDistanceFor`).
2. Turn the camera so the star leaves the view to the side.

## Expected
The star's disc slides out of the frame; the stars and the hull stay as they were.

## Actual
The whole frame turns the star's colour (yellow) with the hull's grey geometry over it. A comet seen against the
star vanished at a certain angle in the same way.

## Notes
Frequency: always, whenever the star's centre comes within about a degree of the camera plane or behind it.

Cause: `drawSun` drew the disc and its glow as screen-space circles of radius `f tan(a) d/z`, which grows without
bound as the centre's depth `z` goes to zero, and `drawGlobe` handed it a radius of 1e9 pixels when the centre was
beside or behind the camera (the B-304 rule for globes, which are ray-traced per pixel and so are safe; the sun was
not). The glow disc of a billion pixels covered the frame; the lens flare's ghosts did the same.

Fix: `SpaceRenderer::drawSun` now takes the unit direction to the star and its angular radius and draws by angle:
each pixel's ray is compared with the star's direction (the disc where the angle is under the radius, the two glow
falloffs outside it, the box the projection of the cone or the whole frame when the cone reaches the camera plane),
in parallel bands. It is right whatever the size and wherever the centre is, in space and on the surface (both suns).
The rays of compact stars and the point of a tiny sun stay screen-space from the projected centre while it is in
front. The lens flare clamps the sun radius to a fifth of the frame. While there, the disc is never dimmer than the
glow-lit sky round it (a low sun inside a saturated corona read as a ring). `unit` renders a companion star from
its parking distance ahead, 70, 95 and 150 degrees off axis: 22.6% of the frame as a disc ahead (22.8% expected),
none and a mean shade under 1 in the other three (`shots/tests/unit_companion_swing.png`).
