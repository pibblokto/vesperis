# B-441: a kept culture grown on its partner

status: resolved 2026-11-19 (before the 1.5.0 release)
severity: S3
area: the culture collection (`game/lab_lists.cpp`: `keepCulture`, `keptPair`, `microscopeStageKept`, the `kept` line; `game/incubator.cpp`:
`flaskPut`, `updateLab`)

## Report

Found 2026-11-19 while measuring the user's slides for their four requests on the laboratory's experiments (R-435 to R-438), in a copy
of their save: two of their four kept cultures, the acid lake's rod and sphere (flask 1's pair), were drawn still on the microscope's
stage, though the collection keeps a species only as it grows. Told to the user with the requests and fixed with them, as recommended.

## Cause

The rod and the sphere grew only on each other's gift: in their flask's broth at 12 C each alone reads NOTHING IT EATS (`growthIn`), fed
GROWS. R-433's collection keeps a species with the medium it grew in, alone: on the stage its drop's cells were made inactive (the kept
drop's `Held::active` was its growth alone, `GF_GROWS`), and put in a flask by itself it would starve. A flask's own drop already held a
hungry culture's cells alive (`microscopeStageFlask`); the kept drop did not.

## Fix

- **Kept with its partner:** a species kept from a flask of two whose growth there was its partner's gift (alone it does not grow in that
  medium, fed it does) is kept with that partner (`KeptCulture::paired`, `partner`; the `kept` line's six fields after its time, read when
  there). Staged, its drop holds both, each at its fed crowd; put in a flask, both go in. Its row says it: "A ROD WITH A SPHERE".
- **Kept before the fix:** a kept culture alone whose flask's pair shows it grew on its partner gets the partner once a load (`keptPair`,
  from the lab's clock).
- **A hungry cell lives on the stage:** a culture's drop says whether its cells live (`Held::alive`: waiting for their warmth, hungry or
  in the dark) or die (too hot, the wrong salt or solvent); R-438's stage then moves them by its warmth.

## Verification (2026-11-19)

- The unit's `lab` group, "a culture grown on its partner kept with it": the scan region's first world with a light-feeder and a
  heterotroph that feed each other in a flask of white light and no energy (150,0,36/0, M4 and M2): the fed one kept with its feeder,
  staged both, put in together, its partner through the save, kept alone and repaired by its flask's pair, alone on the stage alive.
  Before the fix the kept culture had no partner: one species on the stage, its cells inactive.
- The user's save (a copy): kept 0 and 1 alone when loaded; after a frame of the lab's clock "A ROD WITH A SPHERE" and "A SPHERE WITH A
  ROD", each on the stage with the other, both alive and moving at 12 C.
- The battery and the rest: R-438's entry in `docs/decisions/log/7-1.5.0.md`.

## Later

- R-440 (2026-11-19, the collection by hand): the repair of a single kept before the fix (`keptPair`) is retired, as a single is now the
  explorer's to keep (K, alone); a pair is one entry either way round, and a load merges the two this fix kept of one pair (the user's
  rod and sphere: "A ROD + A SPHERE").
