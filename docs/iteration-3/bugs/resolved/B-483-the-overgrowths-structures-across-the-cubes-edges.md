# B-483: the overgrowth's structures across the cube's edges

status: resolved 2026-11-23 (J-16)
severity: S4
area: surface (`surface/overgrowth.cpp`: `growthsOwned`, `growthLatticeAt`)

## Report

KI-489, found in B-461 (2026-11-22): "The overgrowth's structures leave a 16 m strip bare along the twelve edges of the cube map their
lattice is cut from (`growthsOwned` skips a neighbour across a face's edge)." Listed for J-16 on the user's word (2026-11-23): "after
that there should be a J-16 that will basically handle all K-xxx that were created".

## Cause

The structures stand on a lattice fixed to the world: an equal-angle cube map, each face cut into cells about 16 m across. A local 16 m
cell of the landing takes the structures of the world cell its middle lies in and of that cell's eight neighbours on the same face.
Where a neighbour fell off its face, at one of the cube's twelve edges, `growthsOwned` skipped it. A structure of a cell on the other
face whose middle lay in a local cell on this face was then in no local cell: it was never drawn and never in the way.

## Fix

A neighbour off its face is found on the face beyond: its middle is carried on over the edge (the face's angles run on past 45 degrees)
and the cell it lies in there is taken (`growthLatticeAt`). Round a cube's corner two of the eight neighbours come to one cell, so each
cell is read once.

## Verification (2026-11-23)

- The unit's `seeding` group, new check "an overgrowth's structures across the lattice's cube edges": a flesh over a steppe, landed on a
  cube's edge with two faces round it. Of the 297 structures within 80 m (60 of them in the cells along the edge), each stands in exactly
  one local cell. With the neighbour across the edge skipped as before: 10 stood in none.
- A world nobody seeded has no structures, so nothing moves there.
