# B-433: a load or a new expedition noted no visit

status: resolved 2026-10-29 (found 2026-10-27 by U-06's measurement as KI-434; fixed on the user's word)
severity: S4
area: the visit's note (`game/game.cpp`, `frame`'s `visitNoted`; `newGame`; `persistence.cpp`, `load`)

## Report

Found by U-06's measurement (2026-10-27) and offered as KI-434: "A load or a new expedition notes no visit: `visitNoted` is reset
only by an arrival, so the system they put the ship in is not noted (`noteVisit`: its old sights' names made marks, R-408) until
the next arrival." The user: "lets fix KI-423, KI-432, KI-433 and KI-434 one by one starting with 423".

## Cause

The frame notes the system the ship is in once (`if (!visitNoted && sys.valid && state != TITLE)`: `noteVisit`, then the guide
saved). `noteVisit` adds the system to the guide's visits, its class to the classes seen, sets the home and the furthest distance
from it, and makes the old names of its sights marks (R-408). `visitNoted` starts false and only an arrival (`autopilot.cpp`) set it
false again. So the start's own game was noted, but a NEW EXPEDITION from the menu, or a load during play (the slots, the quick
load), put the ship in a system the frame never noted until the ship arrived somewhere else: missing from the visits and the
statistics when the guide had not seen it (a guide newer than the save, or begun again), and its old names left as they were.

## Fix

`newGame` and a load that succeeds set `visitNoted = false`, so the next frame out of the title notes the system they put the
ship in, as an arrival does. The load's own call to `migrateLandmarkNames` stays (the title notes no visit, so a game opened at
the start migrates at once); the second call, the frame's, finds nothing more to do or returns while the first runs.

## Verification (2026-10-29)

A unit check in the `states` group, "a new expedition and a load note the system they put the ship in (B-433)". A new game in its
parking, a frame, a save. The guide's visits emptied, then NEW EXPEDITION from the Esc menu and a frame. Emptied again, then the
save loaded and a frame.
- Before the fix it failed: 0 systems noted after the new expedition, 0 after the load.
- After it, it passes: 1 and 1. The group has 6 checks with 0 failures (B-431's and B-432's among them).

The close of the four, from a clean build (184 compiles, no warning): `tools/prove compare u06` (30 items, 1,793 files, 1,308 s)
finds 28 items the same. The unit differs only by the three checks (410 checks with 0 failures against 407), and the flow by
B-431's two frames. B-433 moved no output: the guides the unit, the flow and the tour write are the same. `tools/prove bench u06
./vesperis_test`, four pairs twice: level (the rows outside the band differ between the two runs and go both ways; this binary
over no budget in the second). `make check` passes. `tools/prove snapshot b433` is the release's base.

In the game: the noting is quiet. The system a new expedition or a load puts the ship in is a visit at once, without an arrival:
counted in the statistics' SYSTEMS VISITED, VISITED beside its star on the HUD, green on the galaxy chart, and passed over by
auto-explore's choice of stars.
