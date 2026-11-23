# B-476: a line's share read at its own warmth

status: resolved 2026-11-23 (J-16)
severity: S4
area: life/seeding.cpp (`seedingRun`, `seedShareAt`)

## Report

KI-499, found in B-449 (2026-11-22): "A line's share and gas are worked out at the warmth the lines before it made: its own warming moves
where it is drawn (its band) but not how much it gives off."

## Cause

`seedingRun` worked out a line's share of its world once, at the warmth the lines before it left. The made air then warmed or cooled the
world with that share's gas. The line's band was drawn at the warmth it made itself, so a line that opened more of its world to itself was
drawn over more ground than its gas was given off for.

## Fix

After the first share, `seedingRun` reads the share again at the warmth the line makes together with the lines before it, all at their
end. Where that share is larger it is kept, and the read repeats, at most six times, until it stops growing. A line that opens its world
to itself spreads and gives off more until it settles. A line whose own warmth drives it back keeps the air it made: its gas stays in
the air, and its share never falls under the share it went down with. Hothouses and snowballs still run away. `seedShareAt` gives a
line's share at any warmth, for the check.

## Verification (2026-11-23)

A new check in the `seeding` group, over the 58 lines of five kinds on twenty worlds that warm or cool their world by 1 C or more:

- On the old code: FAIL. 7 of 58 lived on more of their world at the warmth they made than their gas was given off for. The worst was a
  superhabitable world's line: given off for 0.302 of its world, it lived on 0.469 at the 6.8 C cooler world it made.
- On the new code: 0 of 58, none under the share it went down with, and 9 spread further on the world they made.
