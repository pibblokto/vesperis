# B-486: a ruin's cover at night in its veins

status: resolved 2026-11-23 (J-16)
severity: S4
area: surface (`surface/forms_draw.cpp`: `drawFormItems`; `overgrowth.cpp`: `buildGrowthVeins`)

## Report

KI-492, found in B-466 (2026-11-22): "A ruin's cover glows at night in patches of its edge's noise, not in the ground's veins, and the
cover's edge across a face keeps the wall's light (no gloss there)." Listed for J-16 on the user's word (2026-11-23): "after that there
should be a J-16 that will basically handle all K-xxx that were created".

## Cause

B-466 lit a face wholly under a glowing cover at night through the rasteriser's second bank under the ground's edge noise
(`grainEdge`), so the light lay in soft blobs. On the ground the cover glows dimly all over at night and brightly in its veins (cells
of 9 m whose borders are the veins). A face the cover's edge crosses was drawn with the wall's light, two shades darker under the edge:
no gloss in the sun and no light at night.

## Fix

- A tile of the cover's veins, made the ground's way (`buildGrowthVeins`: four cells of 9 m a side that repeat, a vein where a point's two
  nearest cell points are nearly as far), is the second bank's mask on a face wholly under a glowing cover. At night such a face is lit
  dimly all over and its veins brightly (14 and 50 of the ramp's stops at full glow, as the ground's).
- A face the cover's edge crosses keeps the wall's light above the edge and has the cover's own light below it: its second bank is
  offset by the cover's light less the wall's at the face's middle (its gloss in the sun, its night light), one value a face
  (`FormPolyOut::s2`).

## Verification (2026-11-23)

- The unit's `seeding` group, new check "a ruin's cover at night in its veins and across its edge in its own light": the scene's town
  placed beside it at night, fourteen steps under a glowing wet flesh. Of 5,492 faces drawn wholly under its cover, all 5,492 lit in
  its veins; of 151 faces across its edge, all 151 with its cover in its own light. By the old code none of either.
- `overgrowth ruins`' night frame (`shots/tests/overgrowth/ruins_night.png`) moves: the veins in place of the blobs, the cover's part of a
  face across its edge in the cover's light.
