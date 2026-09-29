---
id: B-207
title: Closing the game on the title screen autosaves the expedition as "in space" and loses the landing
severity: S2
status: resolved 2026-09-27 (found while checking why test runs touched the save files)
reported: 2026-09-27
---

## Where
State / screen: title (after `CONTINUE` was not yet pressed), also `./vesperis --frames N`.

## Steps
1. Quit the game while on the surface (the autosave says `surface 1`).
2. Start the game, close the window on the title screen (or run `./vesperis --frames 40`).
3. Start again and continue.

## Expected
Still on the surface where you were.

## Actual
Back aboard the Stardrifter, parked at the body; the site, position, capsule, buggy and waypoint lines are gone from the autosave.

## Cause
`Game::save` decided "on the surface" from the state, allowing only the menu, help, data, settings and slots overlays to count as surface when they return there; the title (the state after a load) and the guide screens were not in the list, so the exit autosave (`main_raylib.cpp` saves on every exit) wrote `surface 0`.

## Fix
Any state that is not a play state (space, landing map, surface, descent, ascent) is an overlay, and the explorer is on the surface when the overlay returns there (`returnState`). Verified with `./vesperis --frames 40` on a surface autosave: only the `saved` stamp and the ship's orbital position change.

## Related (harness)
Several `vesperis_test` modes (`flow`'s reload, `bench`, `fuzz`, `consistency`, `landmaps`, `settings`, `input`) constructed a `Game` with the test save prefix but the default guide path, so test runs appended to (and once rewrote) the real `guide.txt`; every harness `Game` now names `shots/<mode>_guide.txt` and a key map file of its own.
