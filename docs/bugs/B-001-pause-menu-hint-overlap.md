---
id: B-001
title: pause menu hint "ARROWS + ENTER" overlaps the last option
severity: S3
status: fixed (2026-09-26)
reported: 2026-09-26
reporter: user
---

## Where
State / screen: MENU (Esc from space or surface)
Star / body: any
EPOC: any
Save attached: no

## Steps
1. Press `Esc` in space or on the surface.
2. Look at the bottom of the menu box.

## Expected
Six options and the "ARROWS + ENTER" hint on their own lines.

## Actual
The hint is drawn on top of "QUIT TO DESKTOP".

## Screenshot
`shots/flow_menu.png` (from the scripted flow) shows it.

## Notes
Frequency: always.

## My notes (Claude)
**Cause.** `Game::renderMenu` (`src/game/game.cpp:787-795`) places the six items at `y = 78 + i * 12`, so the last one occupies rows 138-145; the hint is drawn at `FBH - 60 = 140`, inside the same rows. The menu box (`50 .. FBH - 50`) is also too short for seven lines.

**Fix.** Enlarge the box (`44 .. FBH - 36`), keep the items at `66 + i * 12`, draw the hint at `FBH - 46`. While there: dim the hint and add the version line under it. At 2x resolution (M10-04) the layout should be expressed in scale units, so this fix will use a small `menuLayout` helper instead of literals.

**Size.** S. **Check.** `flow_menu.png` regenerated and inspected.

**Fixed 2026-09-26.** `renderMenu` now lays the box out from named constants (top 40, items from 64 every 12 px, hint 20 px above the bottom edge, a version line under it); room for one more item (the Settings entry of M0-02). Verified on `shots/flow_menu.png`.
