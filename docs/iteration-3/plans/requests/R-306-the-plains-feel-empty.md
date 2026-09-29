---
id: R-306
title: Flat ground feels empty: more bushes or something, and terrain that is not a billiard table
status: done 2026-09-28 (O10)
requested: 2026-09-28 (verbal)
related: N2, B-315, B-319
---

## What
"Terrain still feels empty on flat surface, also besides terrain improvements maybe more bushes or something, idk."

## My notes (Claude)
* **The ground:** the relief spectrum gave the plains 0.05 m of relief at 16 m (its exponent is over 1, so the fine
  scales carry almost nothing); `felisianGround` adds hummocks of 0.3-0.5 m at 60 and 130 m and shallow gullies (a
  metre or two deep, 8-14 m wide, on the boundaries of 200 m cells) where it rains, both fading with the sampling
  scale, so they live in the 4 m ring and the nearer 16 m cells.
* **Shrubland** (`drawFloraBand`): 0.5-4 bushes a cell on grass to 320 m (savanna 1.6x, tundra cushions 0.7x and
  small, desert scrub 0.35x on sand), the big ones two clusters; there were 0-2 within 200 m.
* **Mounds:** one cell in six on warm open ground carries 1-3 earth cones 0.8-2.5 m (termite mounds, `limb` with the
  bark tile close by), to 260 m.
* **Tufts** out to 120 m (fewer beyond 60), flowers as before.
* **Stone:** open felisian ground carries twice the boulders (density 0.7), and one cell in sixteen an outcrop of
  three (`drawObjects`).
* Cost: the savanna frame at 4x stays under the forest budget (`bench check`: forest 4x 11.3 ms).
