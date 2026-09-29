---
id: B-317
title: A moon in the sky stands in the ground when boarding the capsule
severity: S3
status: resolved 2026-09-28
reported: 2026-09-28
---

## Where
State / screen: surface, the ascent
Star / body shown in the HUD: any world with a moon low in the sky
EPOC shown in the HUD: n/a
Save attached: no

## Steps
1. With a moon low over the horizon, board the capsule and watch it during the ascent.

## Expected
The ground hides the moon where it stands in front of it.

## Actual
"The moon on the surface was visibly submerged into the planet's surface, basically sticking in it" (the user).

## Notes
`drawGlobe` in sky mode wrote a body's depth in the space renderer's unit, the kilometre, while the terrain writes
metres: a moon 20,000 km away read as 20 km, so every hill beyond 20 km failed the depth test against it and the disc
showed through the ground. Rising in the capsule brings far ground into view under a low moon, which is when it showed.
The sky bodies' depths are metres now (`depthUnit`), so they always lie beyond the farthest terrain; the sun and the
stars test for any depth at all, so the eclipse and the star mask behave as before. `unit` checks that the nearest sky
depth lies beyond the far ring, for the ringed world and for a small body close by (a 1000 km moon at 3.6 radii would
have read as 3.6 km).
