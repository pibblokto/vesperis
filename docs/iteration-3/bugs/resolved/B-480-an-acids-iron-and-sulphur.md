# B-480: an acid's iron and sulphur

status: resolved 2026-11-23 (J-16)
severity: S3
area: life/cultures (`acidFeeds`, `growthIn`), life/seeding.cpp (the place's words)

## Report

KI-507, found in J-14 (2026-11-23): "A cloud species that burns a food (iron, sulphide: about a quarter of them) finds none in its own
clouds: they give dead matter and light, as a vial of them does, and no gene turns it to either."

## Cause

A place gives one food (`mediumOfPlace`): sulphide at a spring or a vent, hydrogen in the rock, iron on a rust world, and dead matter
anywhere else. On an acid world, the biota mostly makes its chemical eaters burn sulphide or iron, the acid waters' foods. Their own
places gave them dead matter, so every one of them went hungry at home, in a flask of their place and on a seeding.

## Fix

An acid's water carries dissolved iron and sulphur. Real examples are the acid waters of old mines, where iron and sulphur eaters live
(Colmer and Hinkle 1947), and a venusian deck's sulphur and iron chloride. So an iron or a sulphide eater now finds its food in any acid
medium, whatever other food the medium gives (`acidFeeds`). The seed page says so: an acid place gives DEAD MATTER, IRON, SULPHIDE.

## Verification (2026-11-23)

A new check in the `seeding` group, over the 24 acid worlds of the scan region with life:

- On the old code: FAIL. All 14 living chemical eaters went hungry at their own place, including all 9 of the clouds. Each of 8 cloud iron
  or sulphide eaters' own lines starved in its clouds ("NO FOOD: IT EATS SULPHIDE, THE PLACE GIVES DEAD MATTER").
- On the new code: none of the 14 is hungry at home and none of the 8 lines starves. 4 of the 8 grow. The other 4 are outgrown at their
  height by a quicker native, as at any place, which is not about food.
