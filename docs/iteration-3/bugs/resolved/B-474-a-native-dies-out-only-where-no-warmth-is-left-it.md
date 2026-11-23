# B-474: a native dies out only where no warmth is left it

status: resolved 2026-11-23 (J-16)
severity: S4
area: life/seeding.cpp (`seedingDiedOut`, `seedVerdictOf`, `seedingRun`, `seedHabitatOf`)

## Report

KI-487, found in J-12 (2026-11-22): "A native a line beat dies out over its whole world once the line covers it: its niches and salts are
read, its band of warmth not, so a vial where the line cannot live holds none either, and a later line's verdict still meets it."

## Cause

`seedingDiedOut` let a native the line beat die out once the line covered its world, if the native lived only in the line's niches and
salts. It never read warmth. A native that also lives in the world's colder or warmer places, where the line cannot live, died out there
too, and no vial of the world held it.

A later line's verdict read the natives at its place from the world as generated (`sampleOf`), so it still met natives an earlier line had
driven out.

## Fix

- While it works out a line's share, `seedHabitatOf` now also keeps the warmth range of the world's waters and of its land, over the same
  96 points (`SeedVerdict::wetLoC` and the rest).
- A beaten native dies out only if none of its own band of warmth lies outside the line's band within that range: its waters' range for a
  water niche, its land's for any other. A native with room of its own lives on, a hundredth of itself where the line lives, as before.
- A seeding now goes down knowing which natives the lines before it have driven out by its time (`seedingRun`), and its verdict meets
  none of them (`seedVerdictOf`'s `gone`).

## Verification (2026-11-23)

A new check in the `seeding` group, over the 175 seedings of nine lines on twenty worlds that J-12's check uses:

- On the old code: FAIL. 260 natives died out, 47 of them with part of their band left them by the world, such as a felisian native of 12
  to 49 C under a line of 7 to 47 C, on a world whose waters run from -2 to 99 C. A later line, let go at the same place once the first
  had covered the world, met 229 of the natives the first had driven out, over 112 seedings.
- On the new code: 210 natives died out, none with warmth left them, and 99 later lines met none of them.

J-12's check of the natives that died out still passes: none die a step early, a vial holds none of them afterwards, and the natives the
line beat are at a hundredth before that.
