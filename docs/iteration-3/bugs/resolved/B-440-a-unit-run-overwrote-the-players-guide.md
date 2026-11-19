# B-440: a unit run overwrote the player's guide

status: resolved 2026-11-18 (before the 1.5.0 release)
severity: S2
area: the harness's games (`harness/test_game.h`: `TestGame`'s paths; `harness/unit.cpp`: the `files` group; `harness/main.cpp`)

## Report

Found 2026-11-18 while reading the user's play files for their five requests aboard (R-427): the repo root's `guide.txt`, the
user's guide, held a test's (413 bytes: one species seen, one KIN line at the expedition's first instant, every statistic 0),
written at 14:03. Told to the user with R-427 and fixed first, as offered, with the test's guide removed so their next game starts
a guide of its own.

## Cause

`TestGame` kept `Game`'s paths (`game.h`): `guide.txt`, `vesperis_save`, `vesperis_settings.txt`, `vesperis_keys.txt`,
`vesperis_pad.txt`, `shots`, `movies`, the player's files in the working directory. Each check set its own by hand, and one did not:
Z-06's sequencer check (`unit_lab.cpp`) loads the reading's line into a second game, whose reading ends, logs its KIN and saves
its guide to `guide.txt` there.

- In `tools/prove`'s scratch directory that file is no one's. Run from the repo root (`./vesperis_test unit`, as the root's
  CLAUDE.md shows it), it is the player's: a unit run there at 14:00 (B-438's, `build/prove/b436/vesperis_test unit`) wrote over
  the user's guide. Their names, marks, log, shards held, species seen and statistics are gone: no copy was kept, and Time Machine's
  disk could not be reached. Their save and settings (10:25) were not written.
- The probe's checks wrote their images the same way, into the player's `shots/` (KI-421): `probe_1` and `probe_2` (`.png`, `.txt`)
  there are the test's, from 14:02.

## Fix

- **A test game's files are the harness's:** `TestGame`'s constructor points every path under `shots/tests/`: `game_guide.txt`
  (its inbox and export beside it), `game_save` (the four saves), `game_settings.txt`, `game_keys.txt`, `game_pad.txt`,
  `game_shots/` (made at the harness's start) and `game_movies/`. A check that keeps its own still sets them; the game reads
  nothing in its constructor, so nothing of the player's is read first.
- **The unit holds it:** its last group, `files`, checks a fresh `TestGame`'s paths, and that the player's files in the working
  directory are as the unit found them before its first group: `Game`'s names (the guide and the two files beside it, the four
  saves, the settings, the keys, the controller's map) and every file in `shots/` (but `tests/`) and `movies/`, by size and time.
- **The battery holds it too:** `tools/prove.items` counts the probe's and the unit's images in `shots/tests/game_shots/`, and none
  in `shots/` (`shots/*.png=0`).

## Verification (2026-11-18)

In scratch directories, the binary before the fix (with the new group) and after it:

| run | before | after |
| --- | --- | --- |
| `unit lab files` | 2 of 16 fail: the paths "NOT: guide.txt, vesperis_save, vesperis_settings.txt, ..."; "guide.txt made" | 0 of 16 fail |
| `unit probe files` | 2 of 17 fail: "shots/probe_1.png made; shots/probe_1.txt made; shots/probe_2.png made; ..." | 0 of 17 fail; the images in `shots/tests/game_shots/` |

`tools/prove compare b438` (35 items, 2,126 files, 1,473 s): 33 the same. `probe` and `unit` differ where the fix moves them, and
nowhere else:
- the probe's images (`probe_1`, `probe_2`, `.png` and `.txt`) are in `shots/tests/game_shots/`, no longer in `shots/`;
- the second game's guide is `shots/tests/game_guide.txt`. The battery reads only what lands under `shots/`, so the old `guide.txt`
  in the working directory was never among its outputs, which is why no compare saw it;
- the unit prints its `files` group: 462 checks, 0 failures (460 before).

**What stays:** the user's guide is lost. The test's `probe_1` and `probe_2` in their `shots/` are left for them to remove (their
own of those numbers, if any, were written over). KI-421 is resolved with this.
