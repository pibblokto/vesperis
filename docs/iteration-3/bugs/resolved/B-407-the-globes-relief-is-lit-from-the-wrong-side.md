# B-407: the globe's relief is lit from the wrong side

status: resolved 2026-10-06
severity: S4
area: space, the globes (and the telescope's plate)

## Report

Found while reworking W-06's telescope (the plate copied the globe's relief shading, and at the plate's fine steps the
shading is strong enough to read): the relief term of `drawGlobe` (M9-15, "slopes facing the star brighten") added
`5 (hE sE + hN sN) / run` to the star's cosine, with `hE` the height rising eastward and `sE` the star's eastward
component. Ground that rises toward the star faces away from it (a ramp rising east faces west: its normal is
`(-slope, 0, 1)`), so the term must be taken off, not added. Every globe's ridges were lit on the side away from the
star and shaded on the side toward it, the crater-or-dome illusion; at the map's 50 km texels the term is a few
hundredths and nobody saw it.

## Fix

`lit -= ...` in both places (the map's term and the plate's, `space/space_view.cpp`), the plate's term held within
+-0.7 so the steepest walls keep a little texture. Nothing generated changes (`GEN_VERSION` 11). The regress: the
space frame `space_S00` is the sun from the first orbit (no globe with a map in it), the scenes are the surface's;
re-run after the fix: 22 hashes, 0 changed (no hashed frame holds a globe with a map at a size where the term shows), so
nothing was re-blessed.

## Verified

`space` renders the telescope's ground frames with the ridges lit on the star's side (`telescope_ground.png`,
`telescope_world_ground.png`); the regress 22 hashes, 0 changed.
