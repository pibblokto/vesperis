---
id: B-312
title: A huge ringed planet in a moon's sky vanishes when it is left or right of the view (two blind spots)
severity: S2
status: resolved 2026-09-28
reported: 2026-09-28
---

## Where
State / screen: surface (a moon's sky; the same code draws the globes in space)
Star / body shown in the HUD: EAMLYUL II-A 77.1 N 3.7 W, SECTOR 176:167
EPOC shown in the HUD: 6011:003.848
Save attached: no (four screenshots: the parent and its rings at headings 169 and 105, nothing at heading 203)

## Steps
1. Land on a moon of a ringed giant and find the parent low in the sky.
2. Turn until its centre leaves the frame to the left or to the right.

## Expected
The limb and the rings stay in view until they leave the frame themselves.

## Actual
The whole planet, rings included, disappears at once: "two blind spots when you moved the camera to the left enough or
to the right enough".

## Notes
`SpaceRenderer::drawGlobe` tested its rays inside a box of radius `rpx = f tan(a) d/z` round the projected centre. The
projection of a sphere is a conic that stretches away from the centre the nearer the centre is to the edge of the view:
45 degrees off axis a 16-degree disc reaches 0.99 f from its centre and the box 0.44 f, so with the centre ten degrees
outside the frame the box was empty and the function returned before it had even set the rings' box. B-304 had fixed
the same class of failure for a centre behind the camera plane. The box is now the exact screen extent of the sphere
(`sphereBox`: the tangent planes through the camera's axes, `(a cz +- R sqrt(a^2 + cz^2 - R^2)) / (cz^2 - R^2)` on the
plane z = 1 for a = cx and a = cy; the whole frame when the sphere reaches the camera plane) united with the projection
of the ring's outer circle (`ringBox`: 48 points of it; the whole frame when any lies at or behind the camera plane).
`unit` renders a ringed world 3.6 radii away with its centre ten degrees outside the frame on either side, in space and
in a moon's sky: the limb shows (17-36 thousand pixels at 1x; nothing before).
