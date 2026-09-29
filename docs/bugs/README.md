# Bug reports

Create one file per bug: `docs/bugs/B-NNN-short-title.md` (NNN = next free number), based on `TEMPLATE.md`.

Severity scale:

* **S1 crash / data loss** — the game exits, hangs, or the save is lost.
* **S2 broken feature** — something cannot be done, or is done wrongly (wrong sun position, landing in the wrong place).
* **S3 visual / audio glitch** — wrong colours, artefacts, popping, clipping, bad text.
* **S4 polish** — feels off, could be nicer, minor inconsistencies.

What helps most: the state you were in (title, space, landing map, descent, surface, a menu), the exact keys pressed, the star or body name shown in the HUD, the EPOC display, and a screenshot (`F12` writes `shots/screenshot_N.png`). If the bug depends on the place, copy `vesperis_save.txt` next to the report (`B-NNN.save.txt`).

Status values I maintain in each report: `new`, `confirmed`, `fixed (commit or date)`, `not reproducible`, `wontfix (reason)`.

`KNOWN-ISSUES.md` lists what I already know about, so you can skip reporting those (or add detail to them).

## Open reports

| Id | Title | Sev | Status |
| --- | --- | --- | --- |
| B-001 | Pause menu hint overlaps the last option | S3 | fixed (2026-09-26) |
| B-002 | F keys intercepted by macOS hardware functions | S2 | fixed (2026-09-26) |
| B-003 | Landing map cursor: no key repeat, Shift too fast | S3 | withdrawn (Shift + arrows works) |
| B-004 | Landing map shows no terrain on single-material worlds | S2 | fixed (2026-09-26) |
| B-005 | Stars too dim from the ship; hard to pick a Vimana target | S2 | fixed (2026-09-26) |
| B-006 | Data sheet on the surface returned to space when closed | S2 | fixed (2026-09-26), found by Claude |
