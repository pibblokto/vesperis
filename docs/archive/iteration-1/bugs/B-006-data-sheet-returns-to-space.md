---
id: B-006
title: opening the data sheet on the surface and closing it puts you back on the ship in orbit
severity: S2
status: fixed (2026-09-26)
reported: 2026-09-26
reporter: Claude (found while tracing the flow test for M0-04)
---

## Where
State / screen: SURFACE -> DATA (`I`, formerly `F2`) -> any key
Star / body: any
EPOC: any
Save attached: no

## Steps
1. Land anywhere.
2. Press `I` (`F2`) for the environment data sheet, then any key to close it.

## Expected
Back on the surface where you stood.

## Actual
The game state jumps to SPACE: the ship view in orbit, the surface abandoned (the capsule is not boarded, no ascent). A save made afterwards records "in space". With F keys unreachable on a Mac (B-002) this was hard to trigger before `I` existed, which is why the flow test's "ascent" step had silently been showing the space view since the first version.

## My notes (Claude)
**Cause.** `updateSurface` switched to `DATA` without setting `returnState`, and the end-of-frame rule `if (state is SYSTEM_LIST or DATA and returnState != SURFACE) returnState = SPACE` kept whatever was there (SPACE after the help screen in space). Closing the sheet returned to `returnState`.

**Fix.** Every overlay entry sets `returnState` explicitly (`DATA` from the surface -> SURFACE, from space -> SPACE, `Tab` list -> SPACE); the end-of-frame rule is gone. The flow test now shows `ascent` in state ASCENT and the slot save made from the surface records `surface 1`.
