# B-489: a kill kept past the landing

status: resolved 2026-11-23 (J-16)
severity: S4
area: game (`game/guide.h`, `guide.cpp`: `DeadRecord`, the `dead` lines; `game/surface_play.cpp`: `deadToLanding`, `deadToGuide`); surface
(`surface/creatures.cpp`: `deadIn`, the feeding)

## Report

KI-494, found in B-470 (2026-11-22): "A landing's kills and falls are kept for that landing alone: the next landing, or a load, plans
every cell whole again, its quarry alive, as KI-317's herds." The user asked for every known issue of 1.5.0 to be handled (2026-11-23:
"after that there should be a J-16 that will basically handle all K-xxx that were created").

## Cause

B-470 kept a landing's dead on the `SurfaceView` (`deadKept`), and `spawnLife` cleared that list at every landing. Nothing wrote it
down, so the next landing, a load or a new game session planned each cell from its hash again, with the killed member in it, and its
remains were gone. KI-317 (a herd forgotten and met again starts where it began) is a different thing and stays as designed: the herds
are planned from the hashes and only the dead are remembered.

## Fix

- The guide keeps the dead by world (`Guide::dead`, `dead` lines): a kill's quarry or a sprouting one fallen, with its seed, its herd's
  cell, where it lies (a unit vector on the body), its species, heading and size, its death on the guide's clock, how long hunters fed
  at it and its stalks. The `clock` line is now written once there is a seeded world or a kept dead, so the deaths' times read back.
- Each frame on the ground, the game copies the landing's new dead into the guide (`deadToGuide`, with the hunters' feeding as it goes).
- At a landing and a load, the game lays the world's kept dead into the landing (`deadToLanding`, `SurfaceView::deadIn`): the landing's
  own herds lose those members, the cells come without theirs, and the remains lie where they fell, aged by the guide's clock since.
- A kept dead is forgotten 64 of the made layer's steps after its death (`DEAD_FORGET_S`, 80 minutes on the guide's clock): its herd has
  young again, and its remains, long scattered, are gone. The guide writes only those not yet forgotten.
- What it costs: a line of about 150 bytes in the guide for each animal seen to die in the last 80 minutes of the clock. A guide with no
  seeding and no kept dead writes no `clock` line and no `dead` line, as before. Older loaders skip the lines, and a loan carries none.

## Verification (2026-11-23)

- The unit's `animals` group, new check "a kill kept past the landing (B-489)": a game on a felisian world, a cell herd's member caught,
  then landed again three steps later on the guide's clock. Before: "the quarry ALIVE AGAIN, its remains GONE; saved and loaded: ALIVE
  AGAIN, GONE". After: "the quarry still dead, its remains where it fell (0.00 cm), BLOATED; saved and loaded: still dead, where it fell
  (0.00 cm); 64 steps on: its herd whole again, no remains; the guide's dead line read back the same, a loan's none, a guide with none
  as before".
- B-470's "a kill kept, remains aged" still passes (within a landing: 0.00 cm, BLOATED, 11 of 35 of the cells' dead staged again).
