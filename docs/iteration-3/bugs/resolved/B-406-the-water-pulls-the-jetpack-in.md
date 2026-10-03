# B-406: the water pulls the jetpack in (you cannot fly over a lake)

status: resolved 2026-10-02
severity: S2
area: surface, movement

## Report

"You can't fly on jetpack over water, it simply pulls you in": flying over a shore on the jetpack, the explorer was
dragged down into the water the moment they were over it.

## Cause

`updateWalking` decided `swimming` from the ground alone: wherever the ground under the explorer lay 0.4 m or more under
the water, they were swimming, whatever their height; the swimming branch then pulled the body to the float level
(1.15 m under the surface) at 4/s and set the jetpack's `onGround`, so the jet cut out. The wading slowdown (speed x0.45
in 1.2 m of water) applied in the air over deep water for the same reason.

## Fix

Deep water is swum once the body is down at its surface: `swimming` needs the feet within 0.3 m of the float level
(0.6 m while already swimming, so a wave does not toggle it), and the air over the water is the air: gravity, the jet
and the air control as over land; the wading slowdown wants the feet on the ground. Falling in from the air drops the
explorer to the float level as before. And the jetpack fires from the water's surface too (afloat, not diving; `Space`
held), lifting the explorer out: a lake crossed by air and run out of heat over was a trap otherwise (no jump from the
water). `unit` flies from a river bank out over the river (14 m off, 1.4 m deep) for ten seconds without swimming, drops
in (swims), and lifts out on Space. Regress unchanged (22 hashes), flow unchanged.
