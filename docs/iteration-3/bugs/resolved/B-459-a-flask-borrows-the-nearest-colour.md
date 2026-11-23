# B-459: a flask past the ship's banks borrows the nearest colour

status: resolved 2026-11-22 (1.5.0, before its release)
severity: S4
area: game (`game/ship_plan.cpp`: `ShipBanks::deal`; `cabin_life.cpp`: `drawIncubator`'s paint)

## Report

KI-455, found in J-01 (2026-11-20): "Twelve flasks each with its own world look ask the room for more free banks than it has: the last
on the shelves dim to the glass's tone (I-03's rule; ice needs none). The incubator's screen draws each in full." The user asked for it
to be fixed (2026-11-22, with the other known issues of the J series).

## Cause

The ship deals its free banks to what asks, nearest first. A flask's look (its dye, its tissue, its sediment) asks a home of its own, so
twelve flasks of different looks with the room's finishes and screens ask more than the pool holds, and the farthest flasks were drawn in
the glass's tone at 0.6 of their shade.

## Fix

After the deal, each home past the pool finds the dealt home nearest its colour within a fifth of the colour cube's diagonal
(`ShipBanks::near`, `bankNear`). A flask past the pool is painted in that bank, so flasks of like colours share one; a flask with no
colour near any dealt one still dims as before. The screens and the finishes keep I-03's rule.

## Verification (2026-11-22)

- The unit's `seeding` group, new check "ship: a flask past the pool borrows the nearest colour": sixteen flasks of eight colours (each
  asked twice, a shade apart) and a pool of eight: eight dealt, eight past it, each of those in the bank of the flask of its colour. By the
  old deal the eight past the pool dimmed.
