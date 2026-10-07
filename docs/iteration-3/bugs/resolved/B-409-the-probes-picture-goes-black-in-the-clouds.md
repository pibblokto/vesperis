# B-409: the probe's picture goes black in the clouds

status: resolved 2026-10-07
severity: S2
area: the probe's descent (the colours of the air and the fog through the decks)

## Report

The user, after X-03, sending a probe into Sashihene VIII (the blue giant Sashihene's ice giant, sector 139 -105 -43,
body 8, R 35,985 km, its one deck the water deck at 5.34-9.62 bar): "I had this beautiful blue gas giant and first it
was beautiful when descending even with moon visible ... but then when I hit clouds surface it just got black and that's
it, I was looking at black screen for a while", five screenshots: the globe from the parking at 89,963 km, the aurora's
curtains over the clouds at 159 km, the moon over the towers in the upper haze at 0.90 atm, and THE WATER DECK at
7.26 atm and 103.5 km deep, black under the link's static.

## Cause

`smoothstep(e0, e1, x)` (`core/types.h`) divides by `e1 - e0`: with the edges one and `x` on them it is 0/0, a NaN, and
`clampd` lets a NaN through (both its comparisons are false). The probe's feed (`Game::renderProbeFeed`) reddens the
light come down to a pressure in two stages, white to the first tint over the first deck (an ice giant's from 0.005 bar)
and on to the second from the first deck's base to the last's; on a giant with one deck the second stage's edges are
both its base, so the tint at the base was a NaN. It coloured the deck's underside and the air's look at the deck's
base, which the sky's and the fog's colours are read toward from the deck's top down, and a NaN colour comes out black:
the whole picture from the deck's top to the end. Above the top the looks between the haze's and the top's were sound,
so the descent was fine until the clouds.

One giant in eight has a single deck (137 of the scan's first 1,106: 70 gas giants, 15 ice giants, 52 brown dwarfs).
X-03's harness frames were all of giants with two and three decks, and its unit checks read the decks' numbers, not
the pictures.

Found on the way: a brown dwarf's fog clipped to one flat orange at its first deck's base, with one deck or more. The
deep's glow lit the fog in full (the glow under the deck times the exposure is about 1.6 there: the exposure counts six
tenths of the glow, for the cavern under the deck), added per unit of the fog's opacity, so the red clipped over nine
tenths of the frame with nothing in it but the rain. X-03 had rendered a brown dwarf's deck in its middle (dark) and
the clear band under it, never the base.

## Fix

* **`smoothstep` with equal edges is a step** (0 under the edge, 1 from it), never a NaN. A one-deck giant's base takes
  the second tint, as the last deck's base does on every giant.
* **The fog's glow** (the fog march of `renderProbeView`, `space/probe_view.cpp`): six tenths of the glow under the deck
  come back to the lens, darkened where the cloud under the point is thick (two taps down the glow's way, against the
  deck's mean structure as the sun's two taps up are; never brightened: with the clear air under the base the taps
  would have lit the fog there twice over), only where the glow counts (over 0.02: a brown dwarf's, a young giant's).
  The test giants' decks have no glow worth it: their pictures and costs are unchanged.

## Verified

The user's descent (`probe 139 -105 -43 8`): the water deck's fog blue-grey with its structure (the frame's middle
reads 80-113 in its channels where it read 5, the static's), its base dark blue in the rain, the deep blue-grey to the
end. A one-deck gas giant (`probe 153 0 32 0`) and a one-deck brown dwarf (`probe 150 0 73 1`) through every stage;
Weesies I's water deck at its base (`probe 150 0 73 0`) reads 141-210 in red where it read 216-254, its pockets dark
against the glow. The unit's new check (`probe: no giant's picture black or clipped flat`, 192 in all): five giants,
three of them with one deck, 21 frames, the darkest frame's mean 14.7, at most 17% of a frame clipped.
`probe stability`'s new place (`glow`, at Weesies I's deck's base): steady (0.070% of the pixels jump: the rain). The
regress unchanged (22 hashes), `flow` exit 0, `bench check` all ok (`probe 2x` 7.8 ms). Nothing generated changes
(`GEN_VERSION` 12).
