# B-505: the ground's budgets set again from their medians

status: resolved 2026-11-23 (J-16)
severity: S4
area: the bench's budgets (`harness/modes_bench.cpp`: `Bench::budgets`, the rows `surface 2x` and `surface 4x`)

## Report

KI-444, found by B-439 (2026-11-18): "`surface 4x` has the tightest budget: 4.5 ms, 1.30 times its median of 3.45 since 1.4.0 began
(F-11 set it at 1.5 times F-10's 2.97). Over it once in 18 runs with the machine in use, as 1.4.0's D-03."

## Cause

The budget was set for a lighter ground. F-11 set both ground rows at about 1.5 times their medians after F-10 (`surface 4x` 2.97 ms,
so 4.5; `surface 2x` 1.82, so 2.8), and the features since (1.4.0's and 1.5.0's) made the ground's frame dearer without its budget
following. Over the 26 runs of `bench check` taken alone in 1.5.0 (the logs in `build/prove/`, R-434 to J-15):

| row | least | median | most | budget | budget over median |
| --- | --- | --- | --- | --- | --- |
| surface 4x | 3.38 ms | 3.44 | 3.79 | 4.5 | 1.31 |
| surface 2x | 2.04 | 2.09 | 2.28 | 2.8 | 1.34 |

The rule is about 1.5 times the median (`rules-harness-proof.md`). One run taken beside other work (Y-03's busy run) drew 4.92 ms at
4x, 1.43 times its median, and went over; at 2x it drew 2.77, just under. B-439 saw the same of the ship's rows: a machine in use slows
a run up to 1.45 times, and a budget under that fails when it does.

The frame did not get slower in 1.5.0: its runs lie in two bands, 3.38 to 3.44 ms and 3.69 to 3.79, and a run falls in either from
the first snapshot to the last, with no trend.

## Fix

Both ground rows are budgeted again at 1.5 times their medians over those 26 runs, as B-439 did the ship's: `surface 4x` 4.5 to 5.2
ms, `surface 2x` 2.8 to 3.1. A change that makes the frame 1.5 times as slow as today's still fails. Not done: a faster ground. The
row times the whole frame of the default site (the terrain's bands, the sky, the flora), and 1.5.0's work did not make it dearer.

The rule (`rules-harness-proof.md`, "The bench"): a budget that the features since brought near its row's time is set again at 1.5
times its median, as one set before its scene was finished.

Other budgets near their times, not changed (their rows were budgeted on purpose at another row's limit): J-10's `city 4x overgrown`
and `capital 4x overgrown`, at the city's and the capital's 8.3 ms (medians 7.2 and 7.0 over seven runs: 1.15 and 1.19 times).

## Verification (2026-11-23)

The numbers above, read from the 27 bench logs of 1.5.0 (`build/prove/*bench*.log`: the busy run set apart). The new limits are timed
at J-16's close by `bench check`, alone.
