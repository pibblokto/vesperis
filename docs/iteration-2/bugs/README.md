# Bug reports, second iteration

Create one file per bug: `docs/iteration-2/bugs/B-2NN-short-title.md` (2NN = next free number from 201), based on `TEMPLATE.md`.

Severity scale (as in the first iteration):

* **S1 crash / data loss** — the game exits, hangs, or the save is lost.
* **S2 broken feature** — something cannot be done, or is done wrongly.
* **S3 visual / audio glitch** — wrong colours, artefacts, popping, clipping, bad text.
* **S4 polish** — feels off, could be nicer, minor inconsistencies.

What helps most: the state you were in, the exact keys, the star or body name in the HUD, the EPOC display, a screenshot (`P` writes `shots/screenshot_N.png` with a caption file), and `vesperis_save_N.txt` next to the report when the bug depends on the place.

Status values: `new`, `confirmed`, `fixed (date)`, `not reproducible`, `wontfix (reason)`.

## Open reports

| Id | Title | Sev | Status |
| --- | --- | --- | --- |
| B-201 | Comets do not appear to move | S3 | resolved 2026-09-26 (N0-01), in `resolved/` |
| B-202 | The surface does not match the world seen from orbit (colours, materials) | S2 | resolved 2026-09-26 (N1), in `resolved/` |
| B-203 | The buggy shakes violently while driving | S3 | resolved 2026-09-27 (N4-06), in `resolved/` |
| B-204 | The steering wheel turns the wrong way and shows squares on it | S3 | resolved 2026-09-27 (N4-06: the cabin is gone), in `resolved/` |
| B-205 | K refuses to recall the capsule after driving; the buggy carries over to other landings | S2 | resolved 2026-09-27 (N4-06), in `resolved/` |
| B-206 | Absurd wind, flickering sun and radial streaks on a thin-atmosphere world | S2 | resolved 2026-09-27, in `resolved/` |
| B-207 | Closing the game on the title autosaves the expedition as "in space" and loses the landing | S2 | resolved 2026-09-27, in `resolved/` |
