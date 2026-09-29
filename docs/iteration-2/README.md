# Documentation, second iteration

The first iteration (`docs/`, milestones M0-M10) is complete. This folder holds the second iteration with the same structure and the same process, started 2026-09-26 from your four complaints:

1. comets are not moving (`bugs/B-201`),
2. the surface does not match what you saw from space, colours first (`bugs/B-202`, milestone N1),
3. felisian worlds are weak: poor vegetation, bad animals (`plans/requests/R-201`, `R-202`; milestones N2, N3),
4. the buggy should look like a buggy, go faster, and show a cabin from the seat (`plans/requests/R-203`; milestone N4).

| Directory | Who writes | What goes there |
| --- | --- | --- |
| `bugs/` | you | Bug reports `B-2NN` from `TEMPLATE.md`; `KNOWN-ISSUES.md` carries the open issues of the first iteration (`KI-NNN`) and the new ones (`KI-2NN`). |
| `plans/` | both | `ROADMAP.md` (milestones `N0`-`N5`, items `Nx-NN`), the `PLAN-*.md` designs, `IDEAS.md`, `PROGRESS.md`, and `requests/` (`R-2NN` from `TEMPLATE-request.md`). |
| `reference/` | me | Nothing new here: the technical notes stay in `docs/reference/` and are updated in place as the code changes (one source of truth, no parallel copies). `reference/README.md` says which notes each milestone touches. |

## Process (unchanged)

1. Report a bug: copy `bugs/TEMPLATE.md` to `bugs/B-2NN-short-title.md`. Screenshots go to `shots/` (`P` in the game), a save next to the report if it depends on the place.
2. Request something: copy `plans/TEMPLATE-request.md` to `plans/requests/R-2NN-short-title.md`.
3. I triage into `KNOWN-ISSUES.md` and `ROADMAP.md`, keep a `status:` line in every file, and log work in `plans/PROGRESS.md`.
4. Nothing in this iteration is implemented yet: I start a milestone when you say "proceed" (or name the milestone), as before.

**Status (2026-09-26):** N0-N4 and N5-01/02/03/05 done; N5-04 (web build, GitHub push) blocked on tooling and access, see `plans/PROGRESS.md`. The third iteration started 2026-09-27 in `docs/iteration-3/`.
