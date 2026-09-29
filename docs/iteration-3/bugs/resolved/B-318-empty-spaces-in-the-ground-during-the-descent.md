---
id: B-318
title: "Empty spaces" in the ground: sawtooth slabs and gaps seen from the capsule on the way down
severity: S2
status: resolved 2026-09-28
reported: 2026-09-28
---

## Where
State / screen: descent (and, hidden behind nearer ground, the surface at any time)
Star / body shown in the HUD: any world
EPOC shown in the HUD: n/a
Save attached: no

## Steps
1. Land anywhere and watch the ground during `SURFACE CAPSULE DESCENDING`.

## Expected
Continuous ground from the capsule down to the horizon.

## Actual
"More visual bugs on surface with 'empty spaces'" (the user, with a screenshot): pale sawtooth-edged slabs and a gap
between them, the sky colour showing through the ground.

## Screenshot
The user's; `shots/tests/capsule_*.png` reproduces it (before: 1-7% of the pixels below the horizon were sky at
every altitude of the descent on the pinned mountain site and on a grassland).

## Notes
The terrain is drawn in rings of 4, 16, 64, 512 and 2048 m cells, and a coarser ring skips the cells the finer ring
covers. B-313 turned that skip into a scalar radius tested against the cell *centres*: a coarse cell whose centre lay
inside the finer ring's radius was skipped whole, so its outer corners, which stick out beyond the finer ring's edge,
were drawn by nobody. Every ring edge was a ring of triangular holes, glaring from the capsule (the notches between
the white slabs of the screenshot are those holes, the slabs the far ring's snow) and hidden on foot behind nearer
ground, except on slopes and skylines. `SurfaceView::cellCovered` now skips a coarse cell only when its farthest
corner lies within the finer ring's guaranteed cover (`coverOf`: the finer radius less 0.71 cells, in metres), which
leaves up to a cell of overlap under the depth bias of B-313. `vesperis_test capsule <scene>` renders the eight
altitudes of the descent and counts sky pixels below the horizon (the beacon beam excepted): 0.00% at every altitude
on both sites now.
