# B-493: a warmed ice sheet's margin melts dark

status: resolved 2026-11-23 (J-16)
severity: S4
area: galaxy/planet_felisian.cpp (`felisianSample`: an ice sheet's ground)

## Report

KI-500, found in B-448 (2026-11-22): "A living world's ice sheet warmed keeps its dome (no height moves): its snow melts to bare ice, the
ice itself stays to its edge as generated."

## Cause

B-448 kept a warmed ice sheet's heights as generated, by the rule that a seeded world's heights and drainage stay as generated (J-04).
Its snow melted to bare ice wherever a made warmth moved the caps, the snow line and the ice's edge off it. The ice itself stayed white out
to its generated edge, so from orbit and on the ground a warmed sheet looked as large as before.

## Fix

The heights still stay as generated: the dome cannot melt down without moving the ground, its drainage and the landing sites. Instead, a
warmed sheet's margin melts dark, as Greenland's does in a warm summer: its dust and meltwater lie on the ice (the dark zone, Wientjes and
Oerlemans 2010). The dark margin reaches inward to where the ice is a share of its full thickness that grows with the warmth, 8% a degree
and 80% at most. The white ice draws back toward the pole as the world warms. A sheet's terminus wall stays bare blue ice.

## Verification (2026-11-23)

A new check in the `seeding` group, the first tundra world's land on 36 by 72 points:

- On the old code: FAIL. At 0, +3 and +6 C the white ice was 790, 742 and 704 points, and no point was dark.
- On the new code: white 790, 679 and 596; dark 0, 63 and 108. No height on the sheets moved (0.0 m at the most).
- B-448's check of every kind's caps still passes.
