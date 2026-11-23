# B-448: every kind's caps follow a made climate

status: resolved 2026-11-22 (1.5.0, before its release)
severity: S3
area: galaxy (`galaxy/planet_grounds.cpp`, `planet_grounds2.cpp`, `planet_internal.h`: `madeCapDeg`, `madeSnowM`, `madeFrostLess`;
`planet_felisian.cpp`: the ice sheets and the glaciers)

## Report

KI-456, found in J-04 (2026-11-21): "J-04's made climate moves only a living world's caps, snow, ice and plants: another kind's caps,
frosts and ice sheets, and a living world's ice sheets, stay as generated whatever the warmth." The user asked for it to be fixed
(2026-11-22, with the other known issues of the J series).

## Cause

Only the living ground (`felisianSample`) read the made warmth (`MadeWorld::warmC`). The other kinds' grounds drew their caps, frost lines
and polar seas from the generated climate alone. On a living world the ice sheets' snow lay "whatever the season", and its glaciers
filled their valleys by the generated warmth.

## Fix

- Every ground's cap edge moves 1.6 degrees toward the pole a degree warmer, as the living world's does: a thin air's and a lichen
  world's caps, an ocean's and a hycean's floes, a steam world's polar seas, an eyeball's pupil (wider warmed), a dry ice world's and a
  cryovolcanic one's caps. An ammonia world's frost line rises 154 m a degree, and the chemotroph's and the nitrogen world's frost fields
  thin. A world nobody seeded, or with no air, has no made warmth, so its edges are read as before to the bit.
- A living world's ice sheets keep their heights (no height moves): where the made warmth moved the caps, the snow line and the ice's
  edge off a sheet, its snow melts to bare ice. A glacier's tongue a made warmth lifts over 2 C melts back to its bed's moraine, its valley
  as generated.

## Verification (2026-11-22)

- The unit's `seeding` group, new check "a made warmth moves every kind's caps": nine kinds (thin air, ocean, lichen, steam, eyeball, dry
  ice, ammonia, hycean, tundra), each 10 C warmer and colder on a grid of 648 points. Before: 1 of 9 moved (the tundra, a living world),
  its ice sheets 0 points of bare ice more. After: 9 of 9, fewer snow and ice points warmed and more cooled (a hycean has none as
  generated and gains some cooled), the tundra's sheets 5 points of bare ice more.
