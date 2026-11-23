# B-466: a ruin's cover with its gloss and its night light

status: resolved 2026-11-22 (1.5.0, before its release)
severity: S4
area: surface (`surface/overgrowth.cpp`: `growthShade`, `growthGlowShade`; `forms_draw.cpp`: `drawFormItems`)

## Report

KI-481, found in J-10 (2026-11-22): "A ruin's cover has no gloss and no night light (the ground's has both); its nodules and drapes are
lit as the walls are." The user listed it with the others to fix (2026-11-22): "Now let's handle all of these".

## Cause

`drawFormItems` lit every piece with C-01's light for walls, the cover's pieces too, and never read the overgrowth's wetness or its
glow (`growthWet`, `growthGlow`). The ground's cover has a wet one's gloss and its veins' light at night, and so do its structures
(`drawGrowth`'s own lighting).

## Fix

The structures' lighting is one helper (`growthShade`: the sky by how much a face looks up, the sun, the lamp, a wet one's gloss toward
the eye; `growthGlowShade`: a fifth of the faces lifted into the ramp's glow at night, pulsing slowly). `drawGrowth` reads it as before,
and `drawFormItems` now lights with it the cover's pieces (nodules, drapes, membranes) and every face wholly under the cover. A face
wholly under the cover also glows at night in patches over it: the raster's second bank under the edge noise, lifted into the glow's
stops. The cover's edge across a face keeps the wall's light, two shades under.

## Verification (2026-11-22)

- `overgrowth ruins`, new frame `ruins_night.png`: the scene's town as if left 300 years ago, at night under a wet flesh whose veins glow,
  fourteen steps on (the industrial test world's cities lie in a day that does not end): 5,492 faces drawn wholly under the cover, 661
  lifted into its glow. Without the fix none: `forms_draw.cpp` never read the glow.
- The day frames: the cover's pieces lit as the ground's structures are (gloss on the wet flesh in the sun).
