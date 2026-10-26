---
name: milestone
description: Run one Vesperis milestone from the user's "proceed" to the report - read STATUS and the milestone's own section, work by the plan's defaults, prove each step, write the lean touch points, report and wait. Use when the user says "proceed" or names a milestone to do.
---

# Run a milestone

## Start

1. `docs/STATUS.md` names the next milestone, its plan and the proof's base. Read the milestone's own section of the plan
   (`grep -n '^### ' <plan>`, then that range), not the whole plan, and the rules of the areas it touches
   (`docs/decisions/rules-<area>.md`).
2. The plan's defaults settle its open questions. Ask only what blocks, one question with the recommended option first;
   ask before Docker or opening the game window, never before the milestone's own `bench check`.
3. The base snapshot exists (`ls build/prove`); with none, `tools/prove snapshot <id>-base` before touching anything.

## Work

- Steps small enough to prove one at a time: a move, an edit, a check, each with the proof it needs (the `prove` skill).
- Long runs: `caffeinate -i`, in the background, the output in a file, alone. Never `make` or `git stash` while one runs.
- A short status line when the user has not heard from you in a while.
- Found and not fixed: a KNOWN-ISSUES row, offered in the report.

## Done when

- Every item of the milestone's done-when, checked, with its numbers.
- The close: from a clean build, the whole battery against the base (`tools/prove compare <base>`: the unit and the flow
  inside it), `bench check` (pairs for a hot path), `make check`; then `tools/prove snapshot <id>`, the next base.

## Touch points (`docs/CLAUDE.md`)

- One entry of at most 4 KB at the end of the current release's log (`docs/decisions/log/`): `## ID (date): title`, the
  decisions and why, then a **Verified** line (the proof, its verdict and numbers), a **Moved** line (what changed where)
  and a **Left** line (what stays open).
- A new or changed rule: its line in `docs/decisions/rules-<area>.md`; a replaced one moves to `superseded.md`.
- The reference notes it changes, edited in place so they read as now; the generated pages made again
  (`./vesperis_test help md > docs/reference/harness/INDEX.md`, `tools/unitchecks.py`, `tools/contents.py`).
- A KNOWN-ISSUES row of at most 300 bytes for what stays; `docs/STATUS.md` rewritten.
- At the close: its row in `docs/ideas/MILESTONES.md` and the iteration's, its request's `status:` line.

## Report

What changed and where; what was proven (the commands and their verdicts, the numbers); what the user decides next. Nothing
is committed unless asked, and then under a random author and committer with no session link. Wait for "proceed".
