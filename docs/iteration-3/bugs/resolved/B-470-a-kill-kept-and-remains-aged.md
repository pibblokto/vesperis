# B-470: a kill kept, and remains aged

status: resolved 2026-11-22 (1.5.0, before its release)
severity: S4
area: surface (`surface/creatures.cpp`: `updateHabitat`, `killCritter`, `restageRemains`, `lifeAnchored`; `surface/surface_view.cpp`:
`update`, `reanchor`)

## Report

KI-471, found in J-08 (2026-11-21): "A herd's cell forgotten and come back to starts whole again: a kill's quarry is back and its remains
gone (KI-317's way), and remains keep the stage they had at the landing." The user asked for it to be fixed (2026-11-22, with the other
known issues of the J series).

Two more found while mapping it: a new anchor (after about 50 km) cleared the flocks and planned none again, so there were no flyers; and
it left the remains in the old frame's metres, so the remains of cells 50 km away showed round the explorer.

## Cause

A cell's herd is planned from the cell's hash each time it comes within 900 m, and nothing remembered what happened to it: the killed
member had the same seed and was planned alive again, and a kill's remains were dropped past 1,300 m and never made again. A remains'
stage was worked out once, from the layer's age at the landing, and the layer is the landing's. The new anchor cleared `flocks`,
`critters` and `herds` but not `remains`, and only the herds had cells to come back from.

## Fix

- The landing keeps its dead on the body (`deadKept`): a kill's quarry or a sprouting one fallen (B-469), with its seed, its herd's cell,
  where it lies as a unit vector, its death on the landing's clock and its hunters' feeding. A cell planned again skips that member, and
  its remains come back where it fell once the explorer is within 900 m.
- The landing's clock (`lifeClock`) runs by the real seconds the guide's clock runs by on the ground, counted in `update` before the
  step's cap. Every half second (`restageRemains`) a cell's dead are staged by their fate's age where they lie at the landing plus the time
  since, and a kill or a fall by the time since it died. A kill goes from fresh to bloated in two steps (ten minutes), and its hunters
  leave once it is bones.
- A new anchor (`lifeAnchored`) plans the flocks round the explorer as the landing did, drops the old frame's remains (the cells lay
  theirs again as they come back) and shows the kept dead again where they fell.

This lasts the landing: the next landing starts every cell whole again, as before.

## Verification (2026-11-22)

- The unit's `animals` group, new check "a kill kept, remains aged" (a blood poison's world): a cell's member caught, the explorer 3 km
  off and back. Before: the quarry alive again, its remains gone; three steps on, none of the cells' 35 dead staged again; 60 km off, no
  flyers and 36 remains of the old frame. After: the quarry still dead, its remains where it fell (0.00 cm off); three steps on, the kill
  bloated and 10 of the 35 staged again; 60 km off, flyers about and no remains of the old frame.
- "remains a pure function of their cell" still holds: 91 remains alike in another order and from a landing 3 km off.
- `regress`: 24 hashes, none changed.
