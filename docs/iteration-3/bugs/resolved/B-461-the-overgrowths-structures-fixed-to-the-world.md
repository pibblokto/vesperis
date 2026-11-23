# B-461: the overgrowth's structures fixed to the world

status: resolved 2026-11-22 (1.5.0, before its release)
severity: S4
area: surface (`surface/overgrowth.cpp`: `growthCellOf`, `growthsOwned`, `forGrowths`, `growthAt`; `flora.cpp`: the trees' test;
`sampling.cpp`: `readSample`)

## Report

KI-476, found in J-09 (2026-11-21): "An overgrowth's structures sit on a landing's own 16 m grid, as rocks and trees do: another landing
nearby draws them elsewhere, and a vial never reads them." The user listed it with the others to fix (2026-11-22): "Now let's handle all
of these".

## Cause

`forGrowths` hashed the landing's own 16 m cells, which are counted in local metres from the landing's anchor. Another landing has
another anchor, so other cells and other hashes: the same ground grew other mounds, ridges and spires. A vial's microhabitat rightly
never reads what a landing lays on its own grid, so it never read them either.

## Fix

The structures now stand on a lattice fixed to the world: the cells of an equal-angle cube map, about 16 m across (each face of the
cube cut into n by n on its two angles). Each world cell places its structures by its own hash: their count by its area, each inside the
cell by its reach, none overlapping another, each heading turned with the cell's axis. They are read into the landing's metres once a
landing (`growthCellOf`, three points of the cell through the site's frame) and given to the local 16 m cell their middles lie in
(`growthsOwned`). What the landing lays is still tested where they are asked for: a building, a road, a street, a work's ground, the
ground drawn and the water (`forGrowths`). Each structure lies in exactly one local cell, and the same in any landing. A structure
can now reach over its local cell's edge, so a tree tests the cells round its own (`forTrees`).

A vial taken on the main thread off a structure says what it was taken from: TISSUE FROM A MOUND, THREADS FROM A CAP, HARD GROWTH FROM A
SPIRE (`growthAt`). A read on the pool (the activity vision) leaves the name, and the microhabitat is the cover's either way, so the
sample is the same square metre's from any landing. A vial off the cover, or off a structure, holds the line that grew it.

## Verification (2026-11-22)

- The unit's `seeding` group, new check "an overgrowth's structures fixed to the world": a flesh over a steppe's forest edge, read from
  a landing there and from one a kilometre north that walked to it. Within 64 m of the first landing, 155 structures, all 155 seen again
  in their places (within 5 cm) and shapes. A vial off one reads "TISSUE FROM A VENT'S LIP", one off the cover clear of them "SOFT
  OVERGROWTH". Without the fix: 152 structures, none seen again; both vials "SOFT OVERGROWTH".
- New check "a vial off an overgrowth holds its line": J-12's record case (a sea line poisoned, its STOP cut, its cells bound) at its
  64th step, eight vials off its cover 1,130 km and more from its place: eight hold its line.
- J-09's check "the overgrowth's structures and trees": nine types on 256 cells, 7,277 structures, a collider at every foot, none walked
  through, 3,778 trees all wrapped, none in a structure.
- The structures move once in the battery's overgrowth frames (another lattice). A world nobody seeded has none.
