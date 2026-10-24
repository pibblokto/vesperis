---
paths:
  - "src/main_headless.cpp"
---

# The harness (`src/main_headless.cpp`, `vesperis_test`)

One 2 MB file until E-03 splits it into `src/harness/` (this note then becomes `src/harness/CLAUDE.md`). Never Read it whole:
`grep -n` for a mode's name or a check's words, then read ranges. E-02 began `src/harness/` (the fingerprint modes): a new mode
can go there, compiled in seconds, with one dispatch line in `main()`.

- **Where things are:** `main()` at the end dispatches every mode by name in one chain, with no table and no list (no argument
  runs `stars`; an unknown mode prints "unknown mode" and exits 0). `testUnit()` (one function of about 6,500 lines) holds the
  unit's checks, partly as group functions such as `historyChecks` and `worksChecks` defined just above it. A `<mode> check`
  branch (`walls check`, `records check`) runs one group alone. The modes are described in `docs/reference/08-testing-and-tools.md`.
- **Outputs** go to `shots/tests/`. The harness reads `tests/regress_baseline.txt` and `src/galaxy/grammar` relative to its
  working directory, so it runs from the repo root. The unit's records check reads the `test_settings.txt` that `settings` writes.
- **Running:** the unit takes a few minutes at about 280% CPU, and its stdout is block-buffered into a log: the tail shows no
  progress, and a segfault (exit 139) loses the last lines, so look at the newest code, not the last line printed. The unit and
  the flow both write `shots/tests/test_guide.txt`: run them one after the other (the regress can run beside the unit). Wall-clock
  checks fail if the Mac sleeps: `caffeinate -i`.
- **Two compiles of this file differ** in a few bytes (clang compiles `testUnit` nondeterministically): prove it by behaviour, and
  map differing bytes to functions (`nm -n`, `cmp -l`) before suspecting a stale object.
- **The scene finder:** `setupSceneForType`'s branch chain catches `wantMat <= -30` (with exclusions for -43..-49) and then
  `wantMat <= -10` before the later branches, so a new negative code needs an exclusion there. A scene far from its type's spot
  must find its daylight again and put the capsule behind the camera; `init` puts the explorer at (6, -4), not at the origin.
- **Unit checks:** a fixed start point can lie in an ocean; an earlier check leaves the drainage switched off
  (`setDrainageEnabled` round `drainageStats`). A check that loops over a region states the count it expects, so a thinner
  galaxy fails instead of testing fewer.
- **The flow:** a step that opens a list row by position breaks when the list's content shifts: open by index through a test
  hook (`testShardsOpenIndex`). `testCabinGoto` takes a pitch (a desk under the window band needs -0.65 from 1.5 m).
- **Test worlds** with a civilisation near home (sectors 150-160 x 20-80): 151 70 1 Seehio II, 152 50 2 Yiothal III; the pinned
  ones (Aieliaalas II, Leileashphail III, Eleinewai I) are in lessons.md. A coordinate without a star or the trait prints nothing.
- **WAVs** the harness writes are checked with python's `wave` module and a slow DFT (no numpy): envelope peaks, the
  autocorrelation pitch of the loudest windows, the DFT's peaks.

More, with the numbers: `docs/reference/lessons.md`, "The harness".
