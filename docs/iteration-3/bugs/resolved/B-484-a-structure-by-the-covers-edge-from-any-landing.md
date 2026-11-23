# B-484: a structure by the cover's edge from any landing

status: resolved 2026-11-23 (J-16)
severity: S4
area: surface (`surface/overgrowth.cpp`: `growthCellOf`, `growthCornerHolds`, `forGrowths`; `flora.cpp`: `forTrees`)

## Report

KI-490, found in B-461 (2026-11-22): "A structure within 16 m of the cover's edge can stand from one landing and not another: whether its
cell is covered reads the landing's 16 m samples (`growthCellHolds`)." Listed for J-16 on the user's word (2026-11-23): "after that
there should be a J-16 that will basically handle all K-xxx that were created".

## Cause

B-461 fixed the structures to the world, but whether a local 16 m cell's structures stood was still read from the landing's own grid
(`growthCellHolds`: the cover at the local cell's four corners, from the landing's 16 m samples). Another landing has another grid. Near
the cover's edge one landing found a cell's four corners covered and another did not, so the same structure stood from one landing and
not from the other.

## Fix

The test moved onto the world's lattice. A world cell's structures stand where the cover holds the cell's four corners, each read at the
corner's own point (`growthCornerHolds`: the cover's share there first, over a half, then the planet function at the 16 m detail the
landing's samples had: its cover laid, not in the water). Each corner is read once a landing. A local cell keeps only the structures of
world cells that stand. What the landing lays (a building, a road, a street, a work's ground, the drawn water) is still tested at each
structure, as before. The trees on a covered forest test the same structures.

## Verification (2026-11-23)

- The unit's `seeding` group, new check "an overgrowth's structures by its cover's edge fixed to the world": a flesh whose cover's edge
  runs through a landing on a steppe's forest edge (its reach halved toward the one whose edge stands there; 35 of 64 points round the
  landing covered). Within 64 m stand 70 structures, and all 70 are seen again in their places from a landing a kilometre north walked
  to it; that landing saw 70 there, all seen from the first. By the old test: 68 from the first landing, 62 of them seen from the
  second, and 62 of its 68 seen from the first.
- B-461's check, away from the edge: 155 structures within 64 m, all 155 seen again, as before.
