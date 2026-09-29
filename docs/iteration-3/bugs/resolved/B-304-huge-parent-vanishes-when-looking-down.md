---
id: B-304
title: A huge ringed parent in a moon's sky vanishes when the camera looks down
severity: S2
status: resolved 2026-09-27 (O0-03)
reported: 2026-09-27 (screenshots: TREYSTAL II-A, the ringed parent filling the sky when looking up, an empty sky at the horizon)
---

## Analysis (Claude)
`SpaceRenderer::drawGlobe` returned at once when the body's centre was behind the camera plane (`cv.z <= 0`). For a small or distant body that is the same as "not visible", but a parent that subtends 60-120 degrees in a moon's sky has its centre behind the camera as soon as the camera looks a little below it, while its lower limb still fills the top of the frame. The whole disc and its rings vanished in one frame and came back in one frame.

## Fix
A globe is skipped only when the whole sphere lies behind the camera plane (`cv.z + R <= 0`). When the centre is behind or beside the camera there is no projection to build a bounding box from, so the rays are tested over the whole frame (the same cost as a disc that fills it); the far-body point branch is skipped in that case. The HUD's `inFront` flag keeps its old meaning.

## Verification
`unit`: a globe seen from 1.15 radii (121 degrees across) with its centre 92 degrees off the view axis still draws 2,884 limb pixels at 1x (0 before). `moonsky` frames unchanged where the parent's centre is in view.
