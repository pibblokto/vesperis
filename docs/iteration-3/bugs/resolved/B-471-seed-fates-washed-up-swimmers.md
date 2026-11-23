# B-471: `seed fates`' washed-up swimmers

status: resolved 2026-11-22 (1.5.0, before its release)
severity: S4
area: harness (`harness/modes_seed_fates.cpp`: `seedFates`, `coastNear`)

## Report

KI-472, found in J-08 (2026-11-21): "`seed fates` draws no frame of the soup seas' swimmers washed up: on its world the lit shores lie far
from the place (the other die-offs' frames show the fate)." The user asked for it to be fixed (2026-11-22, with the other known issues of
the J series).

## Cause

A fate that kills only the waters' animals has no land species dying, so the mode fell back to a random shore site (`waysSite`) that wants
a 0.3 to 1.5 m shallow before the land. On the soup's world it found none. That world turns one face to its sun (its day is its year), and
its place lies on the night side, so the lit coasts are on its far side. Washed-up swimmers also need a habitat cell whose middle or
herd's spot has the water within 150 m.

## Fix

`coastNear` walks 24 rays out from the place to every coast along them (marched, then halved to 20 m), nearest first. It takes the first
that is lit within two of its solar days (its turn against its year), and that has a habitat cell within 600 m whose shore is within 150 m,
read as `cellRemains` reads it. The layer is taken twelve steps after the spread reached it, as for the other die-offs. The frame looks at
the swimmers' remains alone when only the waters die.

## Verification (2026-11-22)

- `seed fates`: soup_seas before: "no land in the light to draw it on"; now "drawn: fates/fate_soup_seas.png (1 animals living within
  300 m; 1 FRESH, 1 BLOATED, 2 SUNKEN, 2 BONES)": a big-jawed fish washed up at the water's edge. The mode draws 78 frames (79 PNGs with
  the sheet; `tools/prove.items` counts 79 with B-469's flock).
- Every other fate's frame is the same bit for bit.
