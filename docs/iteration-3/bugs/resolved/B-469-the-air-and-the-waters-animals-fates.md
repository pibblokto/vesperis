# B-469: the air's and the waters' animals' fates

status: resolved 2026-11-22 (1.5.0, before its release)
severity: S4
area: surface (`surface/bestiary.cpp`: `animalFateOf`, `changeFits`, `animalChangesOf`, `changedBody`; `surface/creatures.cpp`: `planFlocks`,
`swimmersNear`, `drawLife`, the sprouting fall)

## Report

KI-470, found in J-08 (2026-11-21): "Only land herds show a sickness or changed young: a flyer's or a swimmer's fate is its dead alone (a
flock thinned, a spot empty), and a sprouting one climbs but never falls dead on its own." The user asked for it to be fixed (2026-11-22,
with the other known issues of the J series).

## Cause

`animalFateOf` gave a sickness and changed young to land animals alone, and `animalChangesOf` returned no changes for a flyer or a
swimmer. A flock and a shoal carried no fate past their counts, so their members were drawn whole. A sprouting animal walked to the high
ground, and nothing ever ended it there.

## Fix

- A flyer's or a swimmer's survivors are sick and its young changed where the line meets them: their class harmed, or a host's glow or dye
  on the skin, which meets every animal. A blight that starves the plant eaters leaves the air and the water alone. None sprouts in the air
  or the water.
- Their changes are the body's and the hide's alone (`changeFits` per class): no ways, since a flock's and a shoal's members have none of
  their own; a swimmer gets no leg changes and no wool. Short or long legs stunt or lengthen a flyer's wings with them (`changedBody`).
- A flock keeps its fate (`Flock::sickness`, `sick`, `changed`, a giant's or a dwarf's size), and a shoal's members take theirs
  (`SwimmerAt::sick`, `changed`, `sizeK`). Each member is sick or changed by its own hash, with the same salts as a herd's, and drawn
  through the land's look (`fateLook`). A crazed flyer flies erratically, and a crazed swimmer swims twice as fast. The record marks the
  species DYING or CHANGED as it does a herd's.
- A sprouting one falls dead at its time, four to sixteen steps after its fate came where it lives (by its hash): on its high ground once
  it stands there, or a minute past its time wherever it is, its stalks up, kept for the landing (B-470). Its cell planned after that time
  lays its remains on the high ground with the cell's other sprouting dead; the landing's own herds leave out one fallen before the landing.

## Verification (2026-11-22)

- The unit's `animals` group, new check "the flyers' and the swimmers' fates": over 56 seedings at step 40, 23 flyers' and 27 swimmers'
  species now show their sick, and 8 and 6 their changed young; none sprouts. Before, the code gave them none: in j12's `seed fates` every
  flyer and swimmer line reads its dead alone. A sprouting one at its time fell dead, its remains with their stalks, kept for the landing.
- `seed fates`: the flyers' and swimmers' lines now name their sickness and changes (the nerve poison's CRAZED, the gut poison's WASTING,
  a glowing host's GLOWING ANIMALS, the dyed hides' DYED HIDES, a sulphide air's flyer born changed). New frame `fates/fate_dyed_flock.png`:
  a flock of the dyed hides' world, every member dyed red.
- The fates' frames of land herds and remains are the same bit for bit (77 of the old 77).
