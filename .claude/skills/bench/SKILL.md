---
name: bench
description: Measure Vesperis's frame cost against a previous binary with bench check in AB/BA pairs (best-of and median ratios, the 38 budgets, "level"). Use after a change to a hot path, and at a milestone's close.
---

# Measure the frame against a previous binary

- **Once:** `caffeinate -i ./vesperis_test bench check` prints the 38 budget rows (`budget <row> <ms> ms (limit <x>) ok`). A
  milestone's own bench check runs without asking the user.
- **In pairs:** `tools/prove bench <A> <B> [pairs]`. A and B are snapshot names (`build/prove/<name>/vesperis_test`) or binaries,
  e.g. `tools/prove bench e02-base ./vesperis_test`. It copies them to `./vt_pa` and `./vt_pb`, paths of one length (the launch
  path moves rows: P-04), runs `bench check` in the orders AB, BA, AB, BA under `caffeinate -i`, keeps the runs in
  `build/prove/bench/<A>-<B>/`, and prints the table (`tools/benchcmp.py`). Four pairs take about ten minutes.
- **Alone:** in the background, with nothing else running (no make, no harness, no agent building); the user away or not.

## Reading the table

- Each row: the best-of ratio (B's fastest run over A's), the median of the pairs' ratios, A's and B's best in ms.
- **Level:** every row's best-of ratio within 0.97-1.03, or its bests one printed step (0.1 ms) apart.
- **Noisy rows** are named, not hidden: `buggy 2x` moves by itself between runs. A row outside the band that is not noisy is a
  finding: run the pairs again before believing it, then look at what the step changed in that row's path.
- **The budgets:** every run of B within its limits. A run over a budget beside other jobs means nothing; alone, it is a
  regression.
- Hot paths: the frame's passes (space, surface, sky, water, flora, the forms), the radar's scan, the map threads. A change that
  only moves files must be level; a static helper moved out of its file can lose its inlining (the dev build has no LTO).
