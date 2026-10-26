---
id: B-005
title: stars are too dim from the ship; picking a remote target for Vimana flight is hard
severity: S2
status: fixed (2026-09-26)
reported: 2026-09-26
reporter: user
---

## Where
State / screen: SPACE (Stardrifter view), targeting mode (`R`)
Star / body: any
EPOC: any
Save attached: no

## Steps
1. In space press `R` and try to put the crosshair on a star.

## Expected
The star field is bright enough to see candidate stars at a glance and aim at one.

## Actual
Stars are faint dots; finding one to lock is a chore.

## Notes
Frequency: always.

## My notes (Claude)
**Cause.** Two things add up.
1. `drawStarField` (`src/space/space_view.cpp:14`) gives a star an intensity of `38 + 9 log10(L / d^2)` (d in light years), so a sun-like star at 1 LY starts at 38/63 and at 3 LY at 29/63, and only stars above 40 get the four-pixel cross. It then draws one pixel.
2. The mush filter runs twice over the whole frame, and a lone pixel of value V ends up as a soft blob whose peak is about V/4: a 1 LY star is left at roughly 10-13/63, a 3 LY star at about 7/63, barely above black. (`docs/reference/02-rendering.md` already notes that single-pixel stars soften.)
Also, the crosshair lock radius is 14 px and nothing tells you which stars are closest, so the faint dots have to be found by eye.

**Fix.**
1. Stars drawn as 2x2 blocks with a one-pixel halo (bright stars 3x3 plus halo), so the peak after two mush passes is about 0.6 V instead of 0.25 V; brightness curve raised: floor 22 for anything within the 10-sector neighbourhood, 63 within about 1.5 LY. Same change serves the surface night sky (dimmed by the sky as now) and the far-body points of planets (R-001).
2. Targeting mode (`R`) amplifies the field, like the original's field amplificator (roadmap M1-10): intensity x1.6, and the six nearest stars get a small marker with name and distance in LY.
3. `N` in targeting mode cycles the crosshair to the next nearest star (auto-aim), so a target can be locked without hunting; the list is sorted by distance from the ship.
4. Keep the 14 px lock radius; the markers make it discoverable.

**Size.** S (1-2 hours). **Check.** `flow_targeting.png` regenerated: stars visibly brighter, six markers with distances; the flow test locks a target with `N` + `Enter` instead of `testAimAtNearestStar`.

**Fixed 2026-09-26.** `drawStarField`: intensity `46 + 9 log10(L/d^2)` with a floor of 20 inside the neighbourhood, drawn as a 2x2 core with an 8-pixel halo above 34, so a star keeps about 60% of its value through the two mush passes. Targeting mode amplifies the field x1.5 (`SpaceContext::starIntensity`), the six nearest stars get a diamond marker with name and distance in LY, and `N` cycles the crosshair to them (`Game::cycleTargetStar`). The flow test now targets with `R`, `N`, `Enter`.
