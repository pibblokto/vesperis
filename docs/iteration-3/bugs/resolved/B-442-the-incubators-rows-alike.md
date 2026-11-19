# B-442: the incubator's rows alike

status: resolved 2026-11-19 (before the 1.5.0 release)
severity: S3
area: the lab's rows (`game/incubator.cpp`: `flaskSpeciesWords`, `flaskMake`, `incubatorChoices`; `game/sequencer.cpp`:
`sequencerChoices`; `game/lab_lists.cpp`: `keptName`, `keptRow`, `formTraits`, `apartRows`)

## Report

The user, 2026-11-19, asking for names in the incubator (R-442): "it's difficult with unknow + unknown". Their save (a copy): the
flasks read "UNNAMED" and "UNNAMED + UNNAMED", and in the chooser each world's folder listed its two organisms as the same row twice:
"UNNAMED - FROM ACID FROM A LAKE" (the rod and the sphere), "UNNAMED - FROM WET SAND AT THE SEA'S EDGE" (the plate colony and the
amoebae), "UNNAMED - FROM WEATHERED ROCK". Told to the user with the requests and fixed with them.

## Cause

R-432's chooser rows and Z-05's flask labels said a species by its name alone, UNNAMED until named, and the vial it came from: two
species of one vial read the same. The sequencer's chooser did the same; only the collection's rows added the look.

## Fix

- **Its look:** a species with no name is its look in the lab's rows (`flaskSpeciesWords`): a flask "A ROD + A SPHERE" (its first and a
  plus where both do not fit), a chooser's row sized ("A ROD, 2.5 UM BY 1.2 UM - FROM ACID FROM A LAKE"), a kept one's too.
- **Two alike:** two rows of a folder that read the same take their forms' colour and motion (", GREEN, GLIDING"), then a number
  (`apartRows`): over 300 vials, 3 rows alike by look and size, none after.

## Verification (2026-11-19)

- The unit's `incubator` group, "a vial's organisms told apart by their looks": the scan region's first forty living worlds a vial each,
  every species seen: in the incubator's chooser 99 rows repeated and 139 UNNAMED before, 0 and 0 after; the sequencer's the same; a
  flask of two "UNNAMED + UNNAMED" before, "A ROD + A CHAIN COLONY" after.
- The user's save (a copy): their worlds' rows "A ROD, 2.5 UM BY 1.2 UM" and "A SPHERE, 2.5 UM ACROSS - FROM ACID FROM A LAKE"; their
  flask "A ROD +" (A ROD + GATHERING AMOEBAE).
