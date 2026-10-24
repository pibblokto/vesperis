# docs: how the work is planned and recorded

The live places: `iteration-3/plans/PROGRESS.md` (the work log; its `## Next` is the current state), `iteration-3/bugs/` (bugs
and `KNOWN-ISSUES.md`), `iteration-3/plans/requests/`, `ideas/` (`MILESTONES.md`'s ids, `ITERATION-<version>.md`, the plans) and
`reference/` (how the game works: one truth, edited in place). `bugs/`, `plans/` and `iteration-2/` are iterations 1-2's
history, and `README.md` is partly stale (E-04 rewrites it). Over 100 KB, read by range after `grep -n '^#'`: PROGRESS.md,
KNOWN-ISSUES.md and reference/05, 06, 07, 08, 10 (the decisions log, 651 KB) and 12.

## Reference notes
01 architecture, 02 rendering (banks, draw order), 03 coordinates and units, 04 galaxy and systems, 05 the planet function,
06 the surface, 07 game flow, controls and the save format, 08 testing and tools (every harness mode), 09 cookbook (recipes),
10 decisions (a table, then `## ID (date): title` entries, the newest last), 11 notes on the original, 12 civilisations.

## Ids (the next free on 2026-10-23)
Bugs B-429, requests R-424, known issues KI-419. Milestone ids go by letter series in `ideas/MILESTONES.md` (E is the
reorganisation, `ideas/PLAN-reorganisation.md`).

## Touch points
- **A milestone:** a bullet `* **ID (date): title.**` before PROGRESS.md's `## Next`, ending "Docs: <notes>. Nothing
  committed.", and the Next paragraph; a `## ID (date): title` entry at the end of `reference/10-decisions.md` (a generation
  bump also gets a table row and a section at the top of `reference/generations.md`); the reference notes it changes, in place;
  its row in `ideas/ITERATION-<version>.md` ("done <date>") and in `ideas/MILESTONES.md`; its request's `status:` line; a
  KNOWN-ISSUES row for what stays.
- **A bug** (reported in chat; fixed and written together): `iteration-3/bugs/resolved/B-NNN-short-title.md` in the resolved
  style (`# B-NNN: title`, `status: resolved <date>`, `severity`, `area`, `## Report`, `## Cause`, `## Fix`, `## Verification`),
  a row in `iteration-3/bugs/README.md`, a 10-decisions entry, a PROGRESS bullet, the notes changed, a KI row for what stays.
- **A request:** `iteration-3/plans/requests/R-NNN-short-title.md` in R-402's style (`status: done <date>`, `## Request`,
  `## Done`), then the milestone's touch points.
- A bug the user fixed by a redesign stays a B: quote the request in its Report and mark the superseded rule in 10-decisions.
- An issue found and not fixed becomes a KI row, offered to the user. A new series takes a letter no series, bug, request or
  old roadmap used (check `ideas/MILESTONES.md`).

## Writing
- Edit in place, in lines of 140 characters or fewer (99 older lines run past 2,000, which the Read tool cuts).
- Never read a log whole: grep its headings, then read the one entry.
- The music tracks are credited by name only: never their permission or licensing, nor how they were obtained.
- The lessons behind the CLAUDE.md rules, by area: `reference/lessons.md`.
- E-04 reorganises this tree (rules by area, the log by release, STATUS.md); until then PROGRESS.md's Next is the state.
