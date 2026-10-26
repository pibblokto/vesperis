# R-416: the decoder's quick scroll

status: done 2026-10-11 (released in 1.3.0)
milestone: 1.3.0 (after R-415)

## Request

"Can you add a quick scroll in timeline/history with shift pressed so you don't have to click right/left or up/down one event at a time?"

The decoder's timeline (H-06: the records held along the people's eras) moved one record a press of an arrow, its history page one event a
press of `UP`/`DOWN`; the platform's keys do not repeat when held (`IsKeyPressed`), and the pages' `PGUP`/`PGDN` (ten records, eight
events) are under `FN` on a Mac's keyboard.

## What was done

* **Shift with an arrow held runs the cursor** (`Game::shardRunSteps`, `game/shards_screen.cpp`): a step as the arrow (or Shift) goes
  down, then from a fifth of a second on ever faster, 12 a second doubling every half second to 100: 17 records or events in a second's
  hold, a hundred in about two. The run stops at the first and the last (the timeline's arrow alone still goes round from the last to
  the first); letting go of Shift or the arrow stops it, and a new press starts again slow. A slow frame counts a tenth of a second at
  most, so the cursor never leaps.
* **The timeline**: all four arrows, as before (`LEFT`/`UP` back, `RIGHT`/`DOWN` on). **The history**: `UP`/`DOWN`.
* **The hints**: `ARROWS  SHIFT FAST  ENTER READ  TAB PAGE  ESC BACK` on the timeline (was `LEFT/RIGHT A RECORD  ENTER READ ...`),
  `UP/DOWN  SHIFT FAST  ENTER READ  TAB PAGE  ESC BACK` on the history (was `UP/DOWN  ENTER THE RECORD ...`; Enter still opens the record
  that told the event under the cursor).
* The records page keeps its steps (its hint line has no room; its `PGUP`/`PGDN` turn its pages).

## Verified

* **The unit's checks** (in H-06's `historyPageChecks`, also `decoder pages`): on the industrial age world's timeline (34 records held)
  Shift and Left held run back to the first, a tap of Shift and Right moves one, a second's hold 17 more, a long one to the last and no
  further, Right alone from the last goes round to the first; on its history (every held record read: 43 events) the same, Down alone at
  the last staying there.
* **The frames**: `decoder_pages_modern_timeline.png`, `decoder_pages_modern_history.png` (the hints fit, 50 and 51 characters of 53).
* `unit` 0 failures (315: the two new), `regress` unchanged (22 hashes), `flow` 35.1 s, `decoder pages` 0 failures.
