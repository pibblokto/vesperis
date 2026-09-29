---
id: B-314
title: The parent planet in a moon's sky is sometimes gigantic (90% of the sky)
severity: S3
status: resolved 2026-09-28
reported: 2026-09-28
---

## Where
State / screen: surface, a moon of a ringed giant
Star / body shown in the HUD: EAMLYUL II-A and others
EPOC shown in the HUD: n/a
Save attached: no

## Steps
1. Land on the first moon of a giant and look at the parent.

## Expected
"There should be a cap on the closest distance between planets. Sometimes it is pretty, sometimes it takes 90% of the
sky" (the user).

## Actual
From an inner moon the disc took most of the view and the rings the rest.

## Notes
The first moon of a planet orbited at 2.8-4.2 radii (M5-09) while a planet's rings reach 1.85-4.05 radii: a moon
could orbit inside the rings, from where the parent's disc is 42 degrees across (60% of the 70-degree view) and the
rings fill the sky. GEN_VERSION 7 (`galaxy/system.cpp`): the first moon orbits at 4.5-6 radii and at 1.35 times the
ring's outer radius at least, on the same random draw, so nothing else in a system moves. From 4.5 radii the disc is
26 degrees across, a wall of a planet still, and the rings a band across the sky rather than the sky itself. Moon
orbits and periods change everywhere (a save from generation 6 gets the "WORLDS WERE REGENERATED" notice); the
surfaces and the maps do not. The regress systems were re-blessed.
