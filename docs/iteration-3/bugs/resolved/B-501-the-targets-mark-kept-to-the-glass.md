# B-501: the target's mark kept to the glass

status: resolved 2026-11-23 (J-16)
severity: S4
area: game (game/cabin.cpp: `cabinMarks`; game/hud.cpp: `renderSpaceHUD`)

## Report

KI-435, found in I-04 (2026-11-17): "Aboard, the local target's HUD label and mark are drawn over the ship's walls where the body lies
behind them (since I-02 the walls are solid); they could keep to the glass in view (`ShipSight`'s outside box)."

## Cause

The space HUD drew the local target's bracket and name, a belt's rock's bracket and the remote star's box wherever the target's point
fell on the screen. Since I-02 the ship's walls are solid and the outside shows only through the glass, so aboard a target behind the
walls was still bracketed and named over the wall. The glass's box alone would not do: it bounds every pane in view, with the walls,
pillars and frames between them inside it.

## Fix

Once the cabin is drawn, and before the outside's banks are raised, `cabinMarks` reads the frame at each target's point. In the glass's
box the panes keep the space view's pixels in the outside's own bank (`BANK_SPACE`, B-437) or its black; the walls, the pillars and
what stands in the rooms cover the rest with the ship's banks. A body counts as seen if its centre, one of four points round it, or its
point nearest the middle of the frame shows the outside; the remote star by its point. Aboard, the HUD draws a target's bracket, box and
name only where it is seen (`CabinFrame::localOut`, `rockOut`, `remoteOut`; `aboard` set by the frame). The LOCAL and REMOTE lines at
the bottom stay, and off the ship (the radar camera, the telescope) the marks are drawn as before.

A small body right behind a pillar is not marked: the first run of the check found its target behind the great window's middle pillar.

## Verification (2026-11-23)

- The unit's `ship` group, new check "aboard, the target's bracket where the glass shows it alone": parked at the home star's first
  world with the next world out as the target and the bow turned a little right of it, the head turned to it from the great window, each
  of the plan's views and each device's stand. With the marks drawn as the old code drew them, the check failed: the great window 1 of
  1 bracketed, and 40 of 40 views and stands where the target lies out of the glass's box (behind the walls) bracketed too. Now: the
  great window 1 of 1, behind the walls 0 of 40, and 25 of 25 inside the glass's box (through the far rooms' windows too).
- The `ship` group: 16 checks, 0 failures.
- The battery's `ship` item (`ship rooms`, `walk`, `light`, `life`): 21 of its 110 frames lose the parked world's bracket and name where
  they were drawn over the walls (`hangar_middle_1x.png`: 218 pixels, the bracket and UNKNOWN over the hangar's wall); the other 89 are
  the same as the old code's.
