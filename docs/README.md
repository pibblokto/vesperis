# Documentation

**Second iteration (2026-09-26):** the first iteration's milestones are complete; new bugs, requests and the next roadmap live in `docs/iteration-2/` with the same structure. **Third iteration (2026-09-27):** `docs/iteration-3/` (rings, reachability, sectors, belts, comets done; Terrain III under way: the harness and the relief spectrum of `plans/PLAN-terrain-3.md` are in, generation version 6). The reference notes below stay the single source of truth for all of them.

Three areas, three audiences:

| Directory | Who writes | What goes there |
| --- | --- | --- |
| `docs/bugs/` | you | Bug reports. One file per bug, from `TEMPLATE.md`. I keep `KNOWN-ISSUES.md` current and move fixed reports to `bugs/resolved/`. |
| `docs/reference/` | me | Technical notes on how the game works: architecture, rendering, generation, astronomy, units, game flow, tests, how-to recipes, decisions, notes on the original. Read these before changing anything. |
| `docs/plans/` | both | `ROADMAP.md` (milestones I propose), `IDEAS.md` (unscheduled ideas), and `requests/` where you drop feature or improvement requests (from `TEMPLATE-request.md`). |

## Process

1. Report a bug: copy `docs/bugs/TEMPLATE.md` to `docs/bugs/B-NNN-short-title.md`, fill it in, add a screenshot in `shots/` if useful (F12 in game saves one there). Attach `vesperis_save.txt` if the bug depends on where you are.
2. Request a feature or improvement: copy `docs/plans/TEMPLATE-request.md` to `docs/plans/requests/R-NNN-short-title.md`. Reference roadmap item ids (for example `M1-03`) when a request matches one.
3. I triage: bugs get a severity and land in `KNOWN-ISSUES.md` with a status; requests are merged into `ROADMAP.md` with an id, a milestone and acceptance criteria. Both keep a `status:` line so you can see where things are.

## Quick facts

* Build: `make` (needs raylib). Game: `./vesperis`. Headless checks: `./vesperis_test flow|surface|space|maps|moonsky|bench|stars`.
* Frames rendered by the tests land in `shots/tests/` as PNG (since 2026-09-27; `shots/` itself holds your screenshots); the reference docs explain what each test shows.
* The save file is `vesperis_save.txt` in the working directory (text, documented in `reference/07-game-flow-and-controls.md`).
