# B-492: the shallows' stretch along a curving shore

status: resolved 2026-11-23 (J-16)
severity: S4
area: surface (`surface/creatures.cpp`: `shallowsPathOf`)

## Report

KI-497, found in B-467 (2026-11-22): "A shallows' stretch follows the depth's line at its middle only (seven points 4 m apart, one way):
on a sharply curving shore its ends can leave the band and the swimmer is dropped there." The user asked for every known issue of 1.5.0
to be handled (2026-11-23).

## Cause

`shallowsPathOf` read the slope once, at the stretch's middle, and stepped out 4 m at a time along the line square to that slope,
putting each new point back onto the 0.9 m depth across the same slope. Where the shore curved, or a hollow or a bar lay between two
points, the straight chord between them crossed water far deeper or shallower than the 0.3 to 1.5 m the swimmer keeps to: on 10 worlds,
5 of 63 stretches had points off the band, 414 of 5,968 along them, one by 28 m.

## Fix

Each step reads the slope where the last point lies, goes 4 m along the line of that depth there (the way the stretch was going), and
comes back onto the depth across that slope. A step whose chord leaves the band anywhere (read every 0.25 m) is tried again at 2 m; if
that leaves it too, the stretch ends there. A stretch of one point is swum round as before.

## Verification (2026-11-23)

- The unit's `animals` group, new check "the shallows' stretch along a curving shore (B-492)": on 10 worlds with a swimmer of the
  shallows, every stretch of the 100 m cells round a shore site. Before: 63 stretches, 13 turning 20 degrees or more (up to 76), 414 of
  5,968 points off the band, on 5 stretches. After: 59 stretches, 19 turning (up to 104 degrees), 0 of 5,673 points off the band; a
  turning one's member at the surface 100% of a minute's seconds. Four stretches became one point, which the swimmer circles.
- B-467's "the shallows' swimmers" still passes: 28 of 28 shore cells hold their place, 98% of the seconds at the surface (96% before).
- `bestiary ways`: `ways_shallows.png` moved a little (the striped eel of Heereishia II at 139 m where it was at 140 m). `regress`: 24
  hashes, none changed.
