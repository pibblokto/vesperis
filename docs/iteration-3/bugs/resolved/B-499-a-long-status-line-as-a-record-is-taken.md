# B-499: a long status line as a record is taken

status: resolved 2026-11-23 (J-16)
severity: S4
area: game (game/names.cpp: `shardTakenStatus`, `instrumentTakenStatus`; `takeShard`, `takeInstrument`)

## Report

KI-454, found in R-446 (2026-11-20): "A heard sound's status line can run past 53 letters with a long medium ("SOMETHING THAT HUMS OF
THE OLD PEOPLE - 103 OF 200 ON THIS WORLD", 64): the line is centred and cut at both edges (`names.cpp`)."

## Cause

Taking a record said "<ITS OBJECT> OF THE OLD PEOPLE - <N> OF <M> ON THIS WORLD", or "... - CAUGHT BY RADAR (<N> OF <M>)" for one
the radar had heard. The HUD draws the status line centred and holds 53 characters. With "SOMETHING THAT HUMS" the line ran to 64
characters, and 68 when the radar had caught it, so both of its ends were cut. At 400 of 400 ten of the fifteen objects ran past as
they were taken and all fifteen as the radar's. Taking an instrument ("<ITS NAME> OF THE OLD PEOPLE - TO THE CASE") is made the
same way; none of the 130 instruments measured ran past.

## Fix

`shardTakenStatus` and `instrumentTakenStatus` say the line in full where it holds 53 characters, and leave out "OF THE OLD
PEOPLE" where it does not ("SOMETHING THAT HUMS - 103 OF 200 ON THIS WORLD"). The log keeps the whole line.

## Verification (2026-11-23)

- The unit's `states` group, new check "a record's and an instrument's status as they are taken within the 53 characters": every
  medium taken and caught by the radar at 400 of 400 (two peoples' records), and every instrument of the age worlds' peoples.
  - By the old code: 160 lines (30 records' and 130 instruments'), 25 over 53 characters, the longest 68 ("SOMETHING THAT HUMS OF
    THE OLD PEOPLE - CAUGHT BY RADAR (400 OF 400)").
  - With the fix: none over 53. The longest is 53 characters ("A SCROLL OF THE OLD PEOPLE - 400 OF 400 ON THIS WORLD").
