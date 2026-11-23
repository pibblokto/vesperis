# B-478: a small cold world's drill goes deeper

status: resolved 2026-11-23 (J-16)
severity: S3
area: life/seeding.cpp (`deepestKm`, `seedDepthsOf`, `seedHabitatOf`)

## Report

KI-505, found in J-13 (2026-11-22): "The smallest cold worlds stay closed to seeding: a rocky world of 767 km and a carbon one of 903 (`seed
cold`) are still about -75 C at the drill's deepest, 30 km, colder than any line goes."

## Cause

The drill's eight depths were the same on every world, 1 to 30 km. A world's rock warms with depth as its heat flow goes, 30 C a kilometre
on an Earth and less on a smaller world (`deepGradientC`). On a small cold world, 30 km down was still far below the coldest any line
grows at (-45 C, a brine line).

## Fix

The drill's deepest on a world is now the depth where its rock is 10 C warm under its mean: 30 km at least and 120 at most (`deepestKm`).
Its eight levels are the old ones stretched to it, so an Earth's or a Mars's are exactly as before. A deep line's share reads its band to
the world's own deepest.

## Verification (2026-11-23)

A new check in the `seeding` group, over the forty worlds that are not furnaces (a sea under the ice aside): a hydrogen eater tuned for a
depth on the gene editor, as a player tunes one.

- On the old code: FAIL. The 767 km rocky world was still -72 C at 30 km and the 903 km carbon world -77 C, both closed for cold.
- On the new code: no world's rock is closed for cold. The rocky world opens 41 km down and the carbon world 50 km down; 29 of 32 worlds'
  rock is open. The other three (a venusian, a felisian and a hycean world) are closed because their rock is too hot from the first
  kilometre for this line, as a furnace's ground is; KI-505 is about cold.
- J-13's depth checks still pass: the thin-aired world's eight levels are warmer going down, and the europan sea is under its ice.
