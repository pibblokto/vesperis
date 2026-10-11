# B-411: the cabin goes dark and the light does nothing

status: resolved 2026-10-10
severity: S2
area: the Stardrifter's cabin (M2's glass hull)

## Report

The user, after C-17 was committed, with two screenshots (in a Vimana flight with `CABIN LIGHT OFF`; in standby by a carbon
star with `CABIN LIGHT ON`; the cabin as dark in both, the stars showing through the walls, the screens' colours all that was
left): "on my save I randomly had light on/off button not working, cabin of stardrifter became dark/invisible no matter what".

## Cause

The hull was depolarised. The autosave's `cabin` line ends `0 1 0`: the light off, the hull glass, not on the roof. `Y`
depolarises the hull (M2, after the original's onboard device); it lies beside `U` on the keyboard, its status (`HULL
DEPOLARISED - THE WALLS ARE GLASS`) lasts three seconds, and the state is saved with the cabin, so a load comes back to it with
nothing said.

M2 drew the glass by drawing every face of the cabin with `BLEND_DARKEN` (the pixel behind at 45% of its intensity), and the
faces were not only the hull's: the GOES console, the decoder's desk, the radar's and the telescope's sets, the instruments'
case and the front desk are drawn by the same `quad` with the same parameters. A darkened face takes no shade, so the room
went to glass with the walls, and the light, which works through the hull's ramp and the faces' shades, had nothing left to
light: only the screens, the cage's bars, the capsule and the bulb in the ceiling stayed solid. The cabin looked gone, a dark
tint over the stars, and `U` changed nothing to be seen unless the bulb was in view.

## Fix

* **What stands in the room stays solid** (`game/cabin.cpp`): the hull (the floor, the ceiling ring, the walls, the pillars
  and the mullions, the deck's rim seen from the roof) is drawn as glass; the consoles, the back wall's sets, the case and the
  front desk are drawn with the solid hull's parameters, lit by the light as with a polarised hull. The deck's floor is solid
  too (with the grain it lost under a glass hull).
* **The glass catches the light** (`core/raster.*`): `BLEND_DARKEN` takes a `sheen`: the pixel behind darkened as before, or
  the glass's own shade in the hull's bank where that is the brighter (in the darkening's own branch, so the common opaque
  pixel's path is the one it was). The glass's shade is the light's level times 16
  (`GLASS_SHEEN`) times the face's share of the hull's shade: with the light on, the glass shows the room's shape as a faint
  sheen and whatever outshines it shows through; with the light off, it is clear and the room is dark round the stars.
* **The light says the hull is glass**: `U` with the hull glass says `CABIN LIGHT ON - Y POLARISES THE GLASS HULL` (or `OFF`),
  for four seconds.

The key stays where it is and the glass stays saved (KI-390).

## Verified

Two new unit checks (290 in all): toward the GOES console from the cage, the hull's bank holds 51% of the frame polarised and
16% with the hull glass and the light off (the console and the cage, where the console had gone to glass too); the glass
frame is 57.0 bright lit against 27.5 unlit (on 0-255); `U` says `CABIN LIGHT OFF - Y POLARISES THE GLASS HULL`; polarised
and lit again, 51% and within 3% of its brightness. The flow's new `cabin_glass_lit` and its `cabin_glass`, both from across
the room: the desk, the capsule and the cage solid, a sheen on the glass, then the glass clear round the stars with the room
dark. A polarised cabin draws as before: the flow's `cabin` and `roof` frames are byte-identical to the commit's.

`unit` 0 failures (3:42), `flow` exit 0 (38.1 s), the regress unchanged (22 hashes; the cabin is in no hashed frame), `bench
check` within every budget. The first build made the glass a blend of its own, a fifth case in the blends' chain that the
compiler tested before the opaque one: in four alternating pairs of `bench check` against the commit the people's descent ran
3-10 ms slower each time. As a flag on the darkening (its own branch) two pairs came out even (the descent 71.0 and 72.4 ms
against 76.4 and 70.3).
