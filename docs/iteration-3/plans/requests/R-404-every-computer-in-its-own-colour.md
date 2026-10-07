# R-404: every computer in the cabin in a colour of its own

status: done 2026-10-06
milestone: 1.2.0 (with the rework of W-06, on the user's review)

## Request

Reviewing W-06's telescope: "can you make computers at each wall have different unique color so it's easier to find the
one you need?" Every screen in the cabin was the same green phosphor (bank 9), so the console, the flight computer, the
three right-wall screens, the decoder, the radar and the new telescope's set all looked alike from across the room.

## Done

* **A colour per computer** (`SCREENS` in `game/cabin.cpp`, by `cabin.facing`): the GOES console on the left wall amber,
  the flight computer on the front desk the green it had, the right wall's star map screen blue, the landing map screen
  pink, the target data screen cyan, the back wall's shard decoder violet, the signal radar red and the telescope's set
  ice-white. Each screen is drawn in a bank of its own (9 the flight computer's, 21-27 the others, set by `cabinPalette`
  with the same ramp shape as before: black, a quarter, eight tenths, half way to white), so the pixel carries the colour
  through the mush as it did the one green.
* **The prompt in the colour too**: the `E` prompt under the crosshair (`GOES CONSOLE - E TO USE`, `TELESCOPE - E TO LOOK
  THROUGH IT`, ...) is written in the faced computer's colour; the capsule cage keeps the amber.
* Harness: the flow's `cabin_back` (the back wall's three sets) and `cabin_telescope`; `unit` checks that the eight
  computers' colours are pairwise at least 0.3 apart in RGB (`testScreenColours`).
* Docs: 07 (the cabin), 02 (the bank table), 10-decisions (under the W-06 rework).
