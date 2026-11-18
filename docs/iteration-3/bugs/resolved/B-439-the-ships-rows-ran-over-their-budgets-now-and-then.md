# B-439: the ship's rows ran over their budgets now and then

status: resolved 2026-11-18 (KI-443, before the 1.5.0 release)
severity: S4
area: the bench's rows aboard (`harness/modes_bench.cpp`: `Bench::space`'s ship rows, `settleMaps`, `budgets`;
`space/space_view.h`: `mapsMaking`)

## Report

The user, 2026-11-18: "Please commit and handle KI-443". KI-443, found by B-437: "The ship's rows run slow now and then in `bench
check`: `library 4x` past its 2.8 ms in 4 of 26 runs of B-437's build (3.02-3.44 ms), 1 of 16 before, never twice in a row. Steadily
B-437's layer adds 0.1-0.2 ms (2.03 to 2.18)."

## Cause

Not the frame. Three things met in the ship's rows:

- **Their budgets were set on a bare ship.** I-02 budgeted the five rooms at 1.5 times their medians of three runs (the library 1.87
  ms, so 2.8). I-03's finishes and lamps, I-04's life aboard, the laboratory's devices (Z-04 to Z-07), B-435's deal and B-437's layer
  then dressed them. Over eighteen runs now the library takes 2.21 ms and the lab 2.27: their budgets are 1.27 and 1.28 times their
  medians, the tightest of the bench's 46, where the rule's is about 1.5 (`rules-harness-proof.md`). The bridge, the dome, the bridge
  facing the sun and the flasks had come to 1.35-1.43.
- **A machine in use slows a run.** A scratch bench kept every frame's time of the ship's rows, the threads running between frames
  and the maps made, over twelve runs alone:
  - in three of them a stretch of a second or two ran 1.2-1.4 times slower across the ship's rows (the bridge's median 6.06-6.68 ms
    for its 5.64), with no thread of the game's own running beside the frame;
  - other runs were slow elsewhere: `forest 4x` 9.8 ms for its 8.0, `microscope 4x` 11.3 for its 9.8, `buggy 4x` over its budget once;
  - the user's browser was busy, the load average 7.

  A slow stretch lands on any row. A row whose budget is under about 1.45 times its median fails when one lands on it, and the
  library is the first.
- **And the hall timed a map's making.** Its view shows a world first, whose map is made on its thread while the row is timed: in
  12 of 12 runs. The hall's first twenty frames took 2.9-3.4 ms against its own 2.3.

## Fix

- **A row aboard waits for the maps its view starts** before it is timed: `Bench::settleMaps` draws five frames, waits until the maps
  on their threads are made (`SpaceRenderer::mapsMaking`, `TestGame::testMapsMaking`), and draws five frames again. The hall prints
  "(1 map made first)".
- **The ship's seven rows are budgeted again** at 1.5 times their medians of eighteen runs of the dressed ship, as I-02 set them:

  | row | median | budget |
  | --- | --- | --- |
  | bridge 4x | 5.76 ms | 8.2 to 8.6 |
  | hall 4x | 2.38 (its map out of its frames) | 3.6, as it was |
  | library 4x | 2.21 | 2.8 to 3.3 |
  | lab 4x | 2.27 | 2.9 to 3.4 |
  | dome 4x | 4.39 | 6.0 to 6.6 |
  | bridge sun 4x | 2.67 | 3.6 to 4.0 |
  | flasks 4x | 2.45 | 3.5 to 3.7 |

  The microscope's budget, 1.59 times its median, stays.
- **Not done:** folding the raise of B-437's layer into the conversion. It would give back about 0.06 ms of B-437's 0.15, which leaves
  the library's old budget 1.30 times its median, still under the rule's 1.5: the slow runs would stay.

The rules (`rules-harness-proof.md`, "The bench"): a row aboard warms until the maps its view starts are made; a budget set before
its scene was finished is set again from its median once it is, and between, it only falls.

## Verification (2026-11-18)

**The wait:** the scratch bench on the fix, six runs alone. The hall's map was made before its frames in 6 of 6, and no map was made
or finished during any row's frames. The hall's first twenty frames took 2.32-2.48 ms (2.9-3.4 before).

**The budgets:** eighteen runs of the fixed bench, the user's browser busy through some of them.
- The old budgets were broken twice, in one run (the library 3.22 ms, the lab 2.95); the new ones never. The ship's worst run took
  1.46 times its row's median.
- `bench check` alone on the final build, three times: every budget held (the library 2.17-2.64 ms of its 3.3, the hall 2.31-2.64
  with its map made first).
- Elsewhere in those runs, `surface 4x` and `buggy 2x` went over theirs once each (budgets 1.30 and 1.49 times their medians), as
  1.4.0's log records them now and then on a machine in use (D-03's surface 4x). They are not the ship's, and stay as they are.

**Nothing else moved:**
- `tools/prove objects b438`: the engine's and the game's objects, 165 of 165, are b438's.
- Of the harness's 54 objects, `modes_bench.o` differs, and so do the three that vary between compiles (`proof.md`).
- No item of the battery runs the bench, so b438's snapshot stands as the base.

**What stays:** a machine in use can still slow a run past any budget (the rule: run it again alone). `surface 4x` is the tightest
budget now: 1.30 times its median of 3.45 ms, which has stood since 1.4.0 began (KI-444, offered).
