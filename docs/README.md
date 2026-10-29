# Documentation

Where things are written, and how a bug or a request becomes work. `STATUS.md` says where the work stands now.

- `STATUS.md`: the release being made, what is done and what is next, the next free ids.
- `decisions/`: `rules-<area>.md`: the rules in force, a line each, by area; `log/`: every decision with its reasons, by release;
  `superseded.md`; `generations.md`.
- `reference/`: how the game works, a note a topic: 01 architecture, 02 rendering, 03 coordinates and units, 04 galaxy and
  systems, 05 the planet function, 06 the surface, 07 game flow, controls and the save format, 08 testing and tools, 09 cookbook,
  11 the original, `civilisations/`, `harness/`, `lessons.md`.
- `iteration-3/bugs/`: the bug reports (`B-NNN`), `resolved/`, and `KNOWN-ISSUES.md`: what stays open.
- `iteration-3/plans/requests/`: the requests (`R-NNN`).
- `ideas/`: `MILESTONES.md` (every milestone's id), the next release's `ITERATION-<version>.md` once it is planned, the plans and
  ideas not done yet.
- `archive/`: finished: iterations 1 and 2, iteration 3's first plans and its work log, the plans of 1.1.0 to 1.4.0.

## Process

1. **A bug:** tell me in chat, with a screenshot (F12 or P in the game saves one in `shots/`) and the save when it depends on the
   place; or copy `iteration-3/bugs/TEMPLATE.md` to `iteration-3/bugs/B-NNN-short-title.md`. I find the cause, fix it, and
   write the report in the same file: the report, the cause, the fix, the verification.
2. **A request:** in chat, or `iteration-3/plans/TEMPLATE-request.md` copied to `iteration-3/plans/requests/R-NNN-short-title.md`.
   A large one becomes milestones in `ideas/` (`MILESTONES.md`, the iteration's plan).
3. **A milestone** starts when you say "proceed" and ends with a report: what changed, what was proven, what you decide next.
   Its decisions go to the log and the rules, its facts to the reference notes, what stays open to `KNOWN-ISSUES.md`, and the
   state to `STATUS.md`.
4. **Nothing is committed** until you ask.

Every bug, request and known issue keeps a `status:` line. The reference notes are one truth, edited in place when the game
changes; the log keeps why.
