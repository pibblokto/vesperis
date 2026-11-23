# B-475: a line's gas by the room it fills

status: resolved 2026-11-23 (J-16)
severity: S4
area: life/seeding.cpp (`livedOf`, `seedHabitatOf`, `SeedVerdict::room`)

## Report

KI-498, found in B-455 (2026-11-22): "Over `seed kinds`' six lines 13% of seedings show nothing: a sea line on a terminator world lives on
about a hundredth of it and gives a twentieth of its gas (looks alone 19% against the design's third)."

## Cause

A line's gas answers by its share of its world, in full from 0.15 of it (`livedOf`). A terminator world's seas are a thin ring about a
hundredth of its surface, so a sea line that filled them all gave a twentieth of its gas and changed nothing that could be named. On some
of those worlds the habitat's 96 points missed the ring altogether, and the line's share read nothing at all.

## Fix

- A line's room is its kind of place's share of the world at any warmth: its waters of its salts, or its land, or the plants it lives in.
  `seedHabitatOf` counts it over the same points as the share. A line's gas now answers in full from 0.15 of its world, or from all of its
  room where its room is smaller. A line that fills a terminator world's seas gives its whole gas there.
- A line that took lives at least where it went down: its share is never under one of the 96 points, and its room holds that.
- A deep line's and a cloud line's share keeps its own rule (its room the whole world).
- B-449's check of a later line at the made warmth now leaves out a sea at its freezing point as generated, whose own mean lies under the
  -2 C it reads. The terminator worlds' lines now warm their worlds enough to come into that check.

## Verification (2026-11-23)

A new check in the `seeding` group, over `seed kinds`' forty worlds and six lines:

- On the old code: FAIL. 31 of 235 seedings showed nothing, and 5 of 25 gas lines on terminator worlds changed anything.
- On the new code: none of 235 shows nothing, and 25 of 25 gas lines on terminator worlds change their world.
- `seed kinds`' shares are now: 133 seedings harm (57%), 44 change only the looks (19%), 58 make a world more livable (25%), and none show
  nothing. Before they were 49%, 19%, 19% and 13%. 81 end as a made kind, against 65.
- What stays (a new KNOWN-ISSUES row): the looks alone are still 19% against the design's third, and harm is a little over its half.
