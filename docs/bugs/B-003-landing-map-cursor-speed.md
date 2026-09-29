---
id: B-003
title: landing map cursor moves one step per key press; holding does nothing
severity: S3
status: withdrawn (2026-09-26, reporter: Shift + arrows is enough)
reported: 2026-09-26
reporter: user
---

## Where
State / screen: LANDING_MAP (after `C` in orbit)
Star / body: any landable body
EPOC: any
Save attached: no

## Steps
1. In orbit around a landable body press `C`.
2. Hold an arrow key.

## Expected
The cursor keeps moving while the key is held, faster after a moment; a modifier moves in larger steps.

## Actual
One degree per press; holding the key does nothing. (Shift + arrow does move, but six degrees per frame, which is far too fast to aim and is not discoverable.)

## Notes
Frequency: always.

## My notes (Claude)
**Cause.** `Game::updateLandingMap` (`src/game/game.cpp:520-523`) moves on `wasPressed` only, and the Shift path moves on every frame while held (`isDown && shift`) with a 6-degree step, i.e. 360 degrees per second at 60 fps.

**Fix.** Proper key auto-repeat for the map: first step on press, then after 0.3 s repeat at 20 steps per second while held; plain step 1 degree, Shift step 5 degrees (both repeating at the same rate); also accept mouse: click on the map to place the cursor, drag to move it, wheel to nudge latitude. Show the hint "HOLD TO SCROLL, SHIFT = 5 DEG, CLICK TO PLACE". The same repeat helper will serve the system list and menus.

**Size.** S. **Check.** Scripted flow holds an arrow for one second and verifies the cursor moved about 14 degrees (0.7 s x 20 steps).

**Resolution (2026-09-26).** Withdrawn by the reporter: holding `Shift` with the arrows moves the cursor quickly, which is what was wanted. No code change. The key auto-repeat and mouse placement ideas moved to `docs/plans/IDEAS.md` (small delights) in case they are wanted later.
