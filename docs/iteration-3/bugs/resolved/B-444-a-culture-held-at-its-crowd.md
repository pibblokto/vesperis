# B-444: a culture held at its crowd for good

status: resolved 2026-11-20 (with Y-04, before the 1.5.0 release)
severity: S2
area: the cultures' laws (`life/cultures.cpp`: `advance`, its hold at the crowd)

## Report

Found 2026-11-20 while writing Y-04's checks of the switches: a spore former of a budding plan (M2 of a scan-region world, -25 to -9 C,
best -15) grown alone at -14 C stayed AT ITS CROWD from 15 minutes to four hours, with no spores, while the same species at -18 C
sporulated as its decline began. Y-02's and Y-03's flasks had the same: `lab cultures`' y03 print has ten of its 37 curves held at
their crowd past their hold's longest, never resting nor dying.

## Cause

A culture at its crowd holds its reserves `holdS` (5 to 15 minutes, its seed's), then declines. The hold's segment ended at
`t + (holdS - hold)` and added `end - t` back to `hold`: in floating point that sum can come back an ulp short of `holdS`. The next
segment then held again for the rest, a hundredth of an ulp of the clock: its end rounded to its start, a hold of no length, sixteen
times over (the stretch's guard), and the stretch ended still holding. A flask with no event after it held at its crowd for good. Of
324 species of forty worlds at their best that reach their crowd, 96 were held so.

## Fix

The hold's segment knows when it reaches its end (`held`: its end the hold's, within the stretch) and sets `hold` to `holdS` then,
never a rounding short of it. The decline's last segment had its flag already (its cells gone where it ended inside the stretch); the
waking's and the growth's were sound.

The unit's Z-05 check of two in a flask held the mirrored pair's both crowds at half an hour; with their decline in its time, it reads
each one's most over the hour (each to its crowd, as alone).

## Verification (2026-11-20)

- The unit's `lab` group, "a culture's hold at its crowd ends": every living species of forty worlds alone at its best, once at its
  crowd, moved on fifteen minutes and a second later. Before the fix 228 of 324 (96 held, the first Seapheeseeri I M2); after, 324 of
  324. The group's 17 checks pass.
- `lab cultures`: its curves held at their crowd now rest, die or fruit in their hour; the battery against y03 names them (Y-04's
  entry in `docs/decisions/log/7-1.5.0.md`).
