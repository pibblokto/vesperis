# B-467: the shallows' swimmers found

status: resolved 2026-11-22 (1.5.0, before its release)
severity: S4
area: surface (`surface/creatures.cpp`: `swimmersNear`, `shallowsPathOf`)

## Report

KI-467, found in J-07 (2026-11-21): "A swimmer of the shallows shows only where one of its cell's eight places finds water 0.3 to 1.5 m
deep: many shores have none (`bestiary ways` found its first on the eleventh world)." The user asked for it to be fixed (2026-11-22, with
the other known issues of the J series).

## Cause

A 100 m cell whose spot holds a swimmer of the shallows tried eight hashed places and took the first where the water was 0.3 to 1.5 m
deep. On a sloping shore that depth is a strip a few metres wide, so eight random points mostly missed it. The read used the drawn ground
and the water with the tide in, so a spot that was found could move as the explorer walked (the near ring's ground differs from the far
one's) and as the tide turned. The swimmer then circled 5 m round the spot and was dropped wherever its circle left the strip.

## Fix

The cell's place is now found by a search and kept for the landing (`shallowsPathOf`):

- The depth is read on the 16 m ground and the mean water (`groundHeightCoarse`, `waterAt`), so it is the same from anywhere at the
  landing and whatever the tide.
- From the first of the eight places at least 0.9 m deep (the middle of the band), the point at 0.9 m is found by halving toward the first
  place that is shallower, dry ground included: the depth passes 0.9 m somewhere between them.
- With none shallower, the shallowest place over 0.3 m (under 3 m) is taken. With all eight dry, the same is done over a grid of 7 by 7.
- From that point, up to three more points 4 m apart either way follow the line of that depth. The swimmer swims back and forth along
  this stretch, its back up, instead of circling.
- It is dropped only where the drawn water is under 0.12 m (the tide out) or over 3 m.

Other swimmers circle as before, bit for bit.

## Verification (2026-11-22)

- The unit's `animals` group, new check "the shallows' swimmers": on 8 worlds with one, a shore site each, the 100 m cells round it that
  hold both water and dry ground and whose spot is of the shallows. Before: 14 of 28 held its place (50%), and over a minute it was at the
  surface 85% of the seconds. After: 28 of 28 (100%), 96%.
- `bestiary ways`: the shallows' moment on the fourth world tried (a striped eel on Heereishia II, an ocean, in 0.9 m of water), the
  eleventh before. `ways_shallows.png` and the climber's frame (B-468) are the only frames of the `bestiary` item that moved.
- `regress`: 24 hashes, none changed.
