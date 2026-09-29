---
id: B-306
title: The moon's shadow seen on the globe is missing from the landing map
severity: S4
status: resolved 2026-09-27 (O0-04)
reported: 2026-09-27 (verbal: "the shadow from the planet looks good, but when you deploy the capsule this shadow is not visible from the map")
---

## Analysis (Claude)
The globe applies the shadows of moons, the parent and the rings per pixel (M1-08); the landing map applied only the day/night term from the sub-solar direction.

## Fix
`Game::buildShadowMap` computes, per map texel on the day side, the same occluder test as the globe (moons of the body and its parent between the texel and the star, a soft edge over 12% of the occluder's radius) and the ring shadow (the profile's density where the sun ray crosses the ring plane); the map's lighting multiplies its day term by `1 - 0.9 shadow`. The map is cached and refreshed for another body, every 20 s of game time and at most twice a second of real time; the sector zoom gets the same over its own texels.

## Verification
`unit`: the first landable planet with a moon shows a shadow of 1.00 at some point of the moon's orbit; `landmaps` prints the darkest shadow now (1.00 on four maps: the parent's shadow on Brooshaing I-a and on three other moons).
