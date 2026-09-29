---
id: R-303
title: Comets you can land on, with their small size and activity felt
status: done 2026-09-27 (O4)
requested: 2026-09-27 (verbal)
related: O1-02, O4
---

## What
"Comets should be landable though; take into account their small size and effects when you are on them."

## My notes (Claude)
* `PT_COMET` is landable: `C` from the 12-radius parking orbit opens its landing map (a 2-12 km nucleus of ice and rock, GRAV in four decimals), the capsule descends as anywhere.
* Small bodies (radius under 300 km, `SurfaceSite::smallBody`): `toView` places every point on the exact sphere seen from the camera's own vertical (positions foreshorten, the far side curves under), the terrain rings stop at 1.15 radii (past the horizon, which stands 130 m away at eye height on a 5 km nucleus), no floor plane, the local frame re-anchors every 0.35 radii (a hitch of a few frames each time, KI-302).
* Microgravity (escape velocity under 30 m/s, `SurfaceSite::escapeVelocity = sqrt(2 g R)`, 2-10 m/s on a comet): walking and sprinting are capped at 0.3 of it, the jetpack at 0.35 (12 m/s would leave the nucleus), a jump keeps its 6 m apex and floats for minutes, `Ctrl` in the air thrusts down; the buggy refuses (`B`: "TOO LITTLE GRAVITY FOR THE BUGGY"). The HUD shows `GRAV 0.0005 G  ESCAPE 9.2 M/S`, `VENTING 40%` and `CTRL DESCENDS` while airborne.
* Activity (`env.cometActivity`, the tail's own formula from the distance to the star): vents hashed on a 250 m grid on the sunlit side blow columns of dust points that rise for half a minute and bend away from the sun (`drawCometJets`), the coma hazes the sky (brightest toward the sun) and shortens the haze to a few kilometres, the blue ion tail (bank 15) glows some 15 degrees wide round the anti-solar point with the knots streaming outward, so by day it is under your feet and at night it stands overhead. Boulders of dirty ice on the ground (rock density 1.4).
* Verified: `unit` (landable, escape velocity, capped rings, no buggy, sprint under the cap), `flow` (the comet landing: walking at 2.76 m/s with a cap of 2.76, a jump 0.5 m up after 4 s and down after 3 s of Ctrl, frames `comet_surface`, `comet_jump`, `comet_sky`, `comet_sectormap`), scenes `comet_day` (jets) and `comet_night` (the tail), `consistency` (map, globe and ground of a comet within 5 of each other).
