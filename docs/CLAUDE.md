# docs: how the work is planned and recorded

- `STATUS.md`: the state, in one place: the release, what is done, what is next, the proof's base, the next free ids.
- `decisions/`: `rules-<area>.md`, the rules in force by area (`README.md` says which file: read it before changing anything in that
  area). `log/`, every decision by release (`## ID (date): title`, newest last in the current release's file). `superseded.md`.
  `generations.md`, what each `GEN_VERSION` changed.
- `reference/`: how the game works, one note a topic, edited in place. Each note opens with its contents. `civilisations/` (five notes),
  `harness/` (`INDEX.md` and `unit-checks.md` generated, `proof.md`), `lessons.md` (the lessons behind the rules).
- `iteration-3/bugs/` (B files, `resolved/`, `KNOWN-ISSUES.md`) and `iteration-3/plans/requests/` (R files): in use.
- `ideas/`: `MILESTONES.md` (every series' ids), `ITERATION-<version>.md` (the release being made), the plans and ideas still open.
  `archive/`: finished plans, iterations 1 and 2, iteration 3's O plans and work log. The archive is edited only for links.

## Reading

- Over 100 KB (three log files, reference notes 05 to 08, KNOWN-ISSUES.md): `grep -n '^#'` (or the id) first, then a range. A log is never
  read whole: grep its headings (`grep -n '^## B-428' docs/decisions/log/*.md`), then read the one entry.

## Touch points

- **A milestone** (the `milestone` skill): one entry of at most 4 KB at the end of the current release's log: its decisions and why,
  then a **Verified**, a **Moved** and a **Left** line. A rules line for each rule it sets or changes (a replaced one goes to
  `superseded.md`). The reference notes it changes, edited in place, with no dated additions. Generated pages made again (`help md` into
  `reference/harness/INDEX.md`, `tools/unitchecks.py`, `tools/contents.py`). A KNOWN-ISSUES row of at most 300 bytes for what stays.
  `STATUS.md`. At the close: its row in `ideas/MILESTONES.md` and in the iteration's file, and the request's `status:` line. A
  generation bump: a row and a section at the top of `decisions/generations.md`.
- **A bug** (the `bug` skill): its B file is the only write-up, `iteration-3/bugs/resolved/B-NNN-short-title.md` from `TEMPLATE.md`. A
  row in `iteration-3/bugs/README.md`. The notes it changes. A log entry and a rules line only when a rule changed. A KNOWN-ISSUES row
  for what stays. `STATUS.md`'s next id.
- **A request:** its R file from `iteration-3/plans/TEMPLATE-request.md`, with `status:` set when planned and when done. Then its
  milestones' touch points.
- An issue found and not fixed becomes a KNOWN-ISSUES row, offered to the user. A new series takes a letter no series, bug, request or
  old roadmap used (`ideas/MILESTONES.md`).

## Writing

- Lines of 140 characters or fewer (a rules line up to 160). `make check` refuses one over 2,000 (`tools/docwrap.py` wraps).
- The music tracks are credited by name only: never their permission or licensing, nor how they were obtained.
- Write in plain English: the `plain-english` skill (game text, docs, log entries, B and R files, comments, commits, reports).
