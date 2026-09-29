---
id: B-309
title: Ugly flickering round the sun when looking at it (the visor glint arcs)
severity: S3
status: resolved 2026-09-27
reported: 2026-09-27
---

## Where
State / screen: surface, on foot or driving, the sun in view
Star / body shown in the HUD: any
EPOC shown in the HUD: n/a
Save attached: no

## Steps
1. On the surface, look at the sun and move the view across it, or walk toward it.

## Expected
A calm picture; at most a soft glare round the sun.

## Actual
Two bright arcs flicker and spin round the screen centre.

## Notes
Frequency: always with the sun near the centre of the view; a wobble with every head bob otherwise.

Cause: the visor glints (N5-02) were two HUD arcs of the sun mirrored through the centre of the curved glass. Their
angle is `atan2` of the sun's offset from the screen centre, so with the sun near the centre a sub-pixel change of
its position swung the arcs anywhere, and the walking bob moved them every frame. Drawn crisp in the HUD layer over
the melted picture, they also did not sit in the look.

Fix: the arcs are gone. `SurfaceView::drawVeil` draws a veiling glare instead: a broad, faint brightening round the
sun over the finished picture (a glow disc of 0.42 frame widths, 5 shades at the centre with an atmosphere, 3
without, falling off with the square of the distance, no bank change, no depth test) that the mush melts into the
scene. Its strength follows the sun's height, clouds, rain and eclipse, and how much of the disc is still open to
the sky once the ground, the trees and the objects are drawn (25 samples across the disc), so it fades smoothly
behind a ridge or a canopy instead of switching. `vesperis_test stability` measures the change between frames a
thirtieth of a second apart per region (the box round the sun, the sky, the ground) and writes a heat map; the
sunset scene's sun box reads 0.6 out of 255 per frame, the water line accounting for it. KI-212 (no glints from the
cabin) is moot.
