# B-479: a deep line seen where its water comes up

status: resolved 2026-11-23 (J-16)
severity: S4
area: life/seeding.cpp (`looksOf`, `madeTraitWords`), galaxy/made (`madeLives`, `MGT_COLOURED_SEEPS`), galaxy/planetmap.cpp (the land
looks), surface/surface_palette.cpp and space/space_view.cpp (a land look's glow)

## Report

KI-506, found in J-13 (2026-11-22): "A line in the rock shows nothing on the surface, only its gases in the air: no seeps or stains where
its water comes up, and a glowing line under the ice lights no crack at night."

## Cause

J-13 gave a line under the ground one look on the surface: a dyed line under a sea's ice coloured the cracks over it (RED CRACKS). A line
in the rock had no look at all. No deep line's look ever glowed, because only the plants' looks had a night light.

## Fix

- A dyed line in the rock now stains the bare rock where its water seeps up: a new ground look, SEEPS (`MGT_COLOURED_SEEPS`, appended), in
  patches of a few hundred metres within its reach. It takes an ochre seep's colour under its dye's and is named by the dye (RED SEEPS).
- A glowing line under the ice or in the rock now has its cracks or seeps glow at night, in its own stain's colour when it has no dye (GLOWING
  CRACKS, GLOWING SEEPS). The land look's night light is a spread's `landsGlow()` and the layer's `landGlow`. It is drawn in the look's
  bank, on the globe's night side and on the ground, where the look's cells are lit at night in patches of a few tens of metres
  (`madeGlowLand`, as the overgrowth's veins are).
- The layer's key hashes the new glow only when there is one, so other layers keep their keys.

## Verification (2026-11-23)

A new check in the `seeding` group:

- On the old code: FAIL. A red line 5 km down in the thin-aired world's rock made only THICK AIR, with 0 of 76 rock points stained. A
  glowing line under the europan world's ice made nothing on the surface.
- On the new code: the red line makes RED SEEPS and THICK AIR, with 15 of 76 rock points stained and 0 of the rest. The glowing line makes
  GLOWING CRACKS. At the world's night, its cracks' bank reads 134 at its lit stop, against 60 for a red line's cracks.
- J-13's check of a deep line's spread now expects the seeps on the bare rock and nothing else of the ground or the waters; it passes.
- On the ground (`shots/tests/j16/cracks_night.png` beside `cracks_night_before.png`, drawn after the snapshot): the europan world's cracks
  at night have a median luminance of 50, against 11 as generated, and no pixel darker than 16 (70% of them before). The ground's light
  came after the snapshot j16; a compare of eleven items against it (the seed modes, `overgrowth`, `sweeps`, the unit) found them all
  the same, since no battery frame has a glowing deep line at night. `seeps.png` shows a red line's seeps on bare rock by day.
