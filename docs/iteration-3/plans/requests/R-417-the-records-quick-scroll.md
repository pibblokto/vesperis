# R-417: the records page's quick scroll

status: done 2026-10-11
milestone: 1.3.0 (after B-422, the last before the release)

## Request

"in shards decoder also add this fast shift scrolling for records tab"

R-416 gave the decoder's timeline and history the run and left the records page with its steps: its hint line had no room for
`SHIFT FAST` (47 characters of 53, 48 with `* UNREAD`), and its `PGUP`/`PGDN` turn its pages of fifteen rows.

## What was done

* **Shift with Up or Down held runs the records page's cursor** as R-416's run does (`Game::shardRunSteps`): a step as the arrow goes
  down, then 12 a second doubling every half second to 100, through the kind the page shows (`stepShard`, which skips the others), and
  stopping at the first and the last (`stepShard(dir, stop)`); Down or Up alone still goes round from the last to the first. Left and
  Right still change the kind, with Shift or without.
* **The hint**: `ENTER READ  ↔ KIND  SHIFT FAST  TAB PAGE  ESC BACK` (`* UNREAD` in place of `ESC BACK` while a record shown is unread):
  the kind's keys are the font's two arrows (`CH_LEFT`, `CH_RIGHT`), drawn as one; `LEFT/RIGHT KIND` beside `SHIFT FAST` was 59
  characters. 51 and 52 of 53.

## Verified

* **The unit's check** (with R-416's in H-06's `historyPageChecks`, also `decoder pages`): on the industrial age world's records page,
  every kind shown (34 records), Shift and Up held run to the first, a tap of Shift and Down moves one, a second's hold 17 more, a long
  one to the last and no further, Down alone from the last goes round to the first; the music alone shown, the run down ends on the last
  piece, Down alone goes round to the first, the run down and back up ends on it again.
* **The frame**: `decoder_pages_records_hint.png` (the hint fits).
* `unit` 0 failures (319: the one new), `regress` unchanged (22 hashes), `flow` 33.3 s, `decoder pages` 0 failures.
