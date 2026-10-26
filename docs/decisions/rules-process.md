# Rules: process and release

The rules in force for how work is planned, proven, recorded, committed and released, a line each with the entries behind them
(`log/`, by release). A milestone that sets or changes one edits its line here; a replaced one moves to `superseded.md`
(`docs/CLAUDE.md`). The builds and the releases themselves: `rules-process-build.md`.

## Working with the user
- A milestone starts on the user's "proceed", one at a time; a short status line when the user has not heard from you a while [E-01], [E-03]
- The plan's defaults settle open questions; ask only what blocks: one question, the recommended option first [E-01], [E-04]
- Ask before Docker (it starts the user's own containers) and opening the game window; a milestone's own `bench check` runs unasked [E-01]
- Nothing is committed, pushed, tagged or released unless asked, and then only the part asked (the `release` skill) [E-01], [E-04]
- Commits and tags carry a fresh random author, committer and tagger (an 8-hex name, a 12-hex@example.com address), never the user's [E-01]
- No claude.ai/code link or Claude-Session trailer in a commit, tag, note, PR, issue, release or `gh api` write, nor a push of one (`guard.py`) [E-01]
- The report at the end: what changed and where, what was proven (commands, verdicts, numbers), what the user decides next [E-01], [E-04]
- Sound is judged by the user's ear; an agent checks rendered WAVs on paper (pitch, envelope, rms, clipping: `tools/wav.py`) [C-05], [C-07]
- The music tracks are credited by name only: never their permission, their licensing or how they were obtained [E-01]
- Plugins (clangd-lsp, session-report) are the user's to install [E-01]

## Proving and fixing
- A performance or refactoring step keeps every picture, sound and hash, with the workers and with one thread [F-01], [F-02], [F-04..F-08], [F-10], [E-03]
- Floating-point rewrites that move last bits go in one milestone, one bless; a split moves only Linux's (GCC fuses after inlining) [F-10], [E-10]
- A split keeps each call the compiler inlined in one file (`-Rpass=inline` before and after), or times the ones it moves [E-07], [E-08], [E-09], [E-10]
- A split's files share an `_internal.h`: small helpers `inline`, the rest declared, defined once; a file's own stay in it [E-06], [E-07]
- A name a split makes global is checked against every object (`tools/clash.py <header>`): the linker merges a twin silently [E-09]
- A bug's fix comes with its measure: a harness mode or unit check that makes it a number (`matlod`) before a screenshot [B-404]
- Fix at the source: a shared helper's trap in the helper, an impossible state where it is set; never a guard at one call or a reordering [B-409], [B-410]
- One code path: no parallel variant (an HD build, a prototype) stays in the tree; a dropped feature is removed, not hidden [table], [R-414]
- Prototypes and simulations are scratch builds on the build's objects, out of the tree, gone once their decision is logged [table], [H-01], [C-15]
- Long timed runs: `caffeinate -i`, in the background, output to a file, alone; one that met sleep or a closed lid is run again [F-01], [F-08]
- A background job stops at two hours; Spotlight indexing the snapshots stalls proof runs: split long runs [E-02]
- Parallel writers own their files on a tree's copy and a frozen binary; the lead merges by file (`git merge-file`) [H-04 second pass], [H-05], [D-05]
- make reads times to the second: a file a merge or a restore writes gets a fresh time (touch it a second later), or the build is clean [H-05], [P-02]

## Recording the work
- `docs/CLAUDE.md` says what a milestone, a bug or a request writes and where (skills `milestone`, `bug`, `release`); `STATUS.md` is the state [E-04]
- A milestone's entry: at most 4 KB at the end of its release's `log/` file, its decisions and why, then Verified, Moved and Left lines [E-04]
- A rule set or changed gets its line in `rules-<area>.md`, read before changing that area; a replaced one moves to `superseded.md` [E-04]
- A bug's B file is its one narrative; it adds a log entry and a rules line only when a rule changed [E-04]
- What stays unfixed becomes a KNOWN-ISSUES row (KI-NNN, at most 300 bytes), offered to the user [G-01..C-13], [E-04]
- Comments say what is true now and why (units, invariants, traps): a decision pointer a section at most, no bare id or history [E-12]
- Reference notes are edited in place to read as now, never with dated additions; the generated pages are made again [E-04]
- Rules live in `decisions/`, a directory's traps in its CLAUDE.md, the lessons in `reference/lessons.md`, the release's steps in `RELEASE.md` [E-01], [E-04]
- Each source directory's CLAUDE.md names its files and entry points, then its traps (aim 3 KB); `make check` holds every file named [E-01], [E-02]
- No `src/CLAUDE.md` (it would load with every source file): each directory has its note [E-01], [E-03]
- MEMORY.md, loaded every session, is a short index; the memory keeps only what is about the user, the history in the docs and git [E-01]
- A log is never read whole: grep its headings, then read the entry; a whole Read of a file over 100 KB is refused (`guard.py`) [E-01]
- A new series takes a letter no series, bug, request or old roadmap used (`ideas/MILESTONES.md`) [E-01], [E-04]

[table]: log/0-1.0.0.md
[G-01..C-13]: log/1-1.1.0.md
[B-404]: log/1-1.1.0.md
[C-05]: log/1-1.1.0.md
[C-07]: log/1-1.1.0.md
[B-409]: log/2-1.2.0.md
[B-410]: log/2-1.2.0.md
[H-01]: log/3-1.3.0.md
[C-15]: log/3-1.3.0.md
[H-04 second pass]: log/3-1.3.0.md
[H-05]: log/3-1.3.0.md
[R-414]: log/3-1.3.0.md
[F-01]: log/4-1.3.1.md
[F-02]: log/4-1.3.1.md
[F-04..F-08]: log/4-1.3.1.md
[F-08]: log/4-1.3.1.md
[F-10]: log/4-1.3.1.md
[P-02]: log/6-1.4.0.md
[D-05]: log/6-1.4.0.md
[E-01]: log/6-1.4.0.md
[E-02]: log/6-1.4.0.md
[E-03]: log/6-1.4.0.md
[E-04]: log/6-1.4.0.md
[E-06]: log/6-1.4.0.md
[E-07]: log/6-1.4.0.md
[E-08]: log/6-1.4.0.md
[E-09]: log/6-1.4.0.md
[E-10]: log/6-1.4.0.md
[E-12]: log/6-1.4.0.md
