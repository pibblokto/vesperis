# B-446: a switched line in a predator cycle

status: resolved 2026-11-20 (1.5.0, before its release)
severity: S4
area: cultures (`life/cultures.cpp`: `runCulture`'s cycle, `cycleCross`)

## Report

KI-452, found in Y-04 (2026-11-20): in a flask where a predator and its prey cycle, a strain with a gene switch kept the switch state
it had when the stretch began. As the populations rose and fell, its "crowded" state never changed, so its growth rate and what its
drop showed stayed wrong until the next event in the flask. The user asked for it to be fixed (2026-11-20: "Fix KI-451 and 452").

## Cause

Outside a cycle, `advance` splits growth where the population crosses the quorum share (the "crowded" threshold) and picks the strain's
variant for each part. The predator-prey cycle is one closed-form law per stretch: `runCulture` built it once from the variant at the
stretch's start and never looked at the numbers again.

## Fix

The cycle is now built in parts. Each part reads every switched strain's state from its current numbers: crowded when past its quorum
share; light and cold fixed by the stretch; never starving, since both species grow under the cycle's law. `cycleCross` finds where
the next part starts: it samples the closed form at 1/64 of a period and halves down to the crossing. The search stops once the swing
has decayed too far to reach the threshold again. If a strain cannot grow in its new state, the cycle breaks there and each species
goes on from its numbers, as before. A pair with no switched strain takes the old path unchanged.

When a cycle breaks, a switched predator's quorum share is measured against its own crowd again (it was the cycle's).

## Verification (2026-11-20)

- The unit's `lab` group, new check "a switched line in a predator's cycle follows its numbers": a rod with a moved gene (3%) set WHEN
  CROWDED, with a grazer. Without the fix: its state right at 362 of 460 samples, one cycle part. With it: 460 of 460, seven parts
  (four crowded), each growing at its state's rate (x0.97 crowded). The group's 18 checks pass.
- Unswitched pairs: the unit's "two in a flask, the four ways" check unchanged; `lab cultures`' pairs the same in the battery.
