# B-405: invisible walls in a town (the jetpack cannot fly over a low wall)

status: resolved 2026-10-02
severity: S2
area: surface, collision

## Report

Playing on a desert world with a people's town (C-01), the explorer met "a lot of invisible walls": a small wall, or
something like one, that the jetpack could not fly through or over.

## Cause

A collider was a disc on the ground plan with no height (`SurfaceView::Collider` was `x, z, r`): rocks over 0.6 m, tree
trunks, logs and, since C-01, every standing piece of a settlement as a row of discs along the wall. The explorer's
collision read the discs whatever the feet's height, so a wall stub of a metre stopped the jetpack ten metres up, and
a wall whose foot stands on the slope below (each piece sits at its own foot, KI-345) stopped an explorer walking on
the ground above its top: a wall nothing could be seen of. Trees and boulders had the same fault, unnoticed because a
trunk is tall and a boulder is small.

## Fix

Every collider carries the span it blocks, in absolute metres (`y0..y1`): a rock from its sunk base to the peak's
reach (1.15 sizes over the ground), a trunk to the tree's height, a log to a diameter over the ground, a monolith to
its drawn height, a settlement's piece from its foot (the ground where it is drawn, `groundHeight` at the piece's
centre) to its top. The explorer skips a collider whose top is under the feet (by 0.1 m) or whose foot is above the
head (1.6 m up): the jetpack clears a wall it has risen above, a slope's buried wall is walked over, a lintel never
blocked. The buggy skips what lies under its wheels or over its roll bar; `findOpenRun` skips what stands under the
ground of the sample. The unit's town check (`settlements: the colliders leave the rooms and doorways open and stand
as tall as their walls`) now also asserts every piece's collider spans 0.85-30 m from a foot within 8 m of its ground,
and reports the tallest (11.4 m, a tower, on Aieliaalas II's town). Landing on a wall's top from above is not
supported: the ground is the terrain, so the explorer sinks through the wall and is pushed out of it sideways
(KI-345's plinth, for C-08). Regress unchanged (22 hashes), flow unchanged.
