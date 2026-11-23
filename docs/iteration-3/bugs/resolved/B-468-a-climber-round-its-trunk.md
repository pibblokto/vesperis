# B-468: a climber round its trunk

status: resolved 2026-11-22 (1.5.0, before its release)
severity: S4
area: surface (`surface/creatures.cpp`: `updateLife`'s climb, `climbSide`; `surface/animal_mesh.cpp`: `poseAnimal`; `surface/flora.cpp`:
`trunkGirth`, `trunkOf`)

## Report

KI-469, found in J-07 (2026-11-21): "A climber clings to a flat plane touching its trunk: on a thin trunk a wide climber's side feet stand
off the bark, and two of a herd can share a side." The user asked for it to be fixed (2026-11-22, with the other known issues of the J
series).

## Cause

A climber was posed on level ground and turned so its belly faced the trunk. Its feet lay on the flat plane touching the trunk, so a foot
to the side stood off the round bark, and past the trunk's radius it stood on nothing. The trunk it measured was the collider's (0.12 m
and 2% of the tree's height), not the drawn one: a family's girth, a wrapped trunk's sheath, the taper to the top and the lean were all
ignored. Its side was the side it came from plus a hash of up to 46 degrees, and herd mates running from one threat to one trunk came
from one side.

## Fix

- The trunk is the drawn one. `trunkGirth` is the tree drawing's own girth formula (moved out of `drawTree`, which reads it unchanged), and
  `trunkOf` gives a climber the radius at the foot and at the top (a wrapped trunk's sheath round both, a stone stump's girth) and the
  top's lean. A climber follows the axis and the taper as it goes up (`trunkPoint`, `trunkRadius`).
- The pose takes the trunk's radius there (`AnimalPoseIn::trunkR`). The level ground it is posed on is then the bark's round under it,
  and each foot grips the bark within 0.92 of the girth's sides, so a wide climber on a thin trunk holds on round it.
- `climbSide` keeps herd mates apart: a climber takes the side it came from only where it is clear of those already on the trunk by its
  width with its legs out at the thinner of their two girths, or a body's length up or down. Else it tries the next side either way. With
  no side clear, it climbs a body's length under the lowest (over the highest where that leaves too little trunk). The choice reads the
  climbers in their order, so it is the same every time.

## Verification (2026-11-22)

- The unit's `animals` group, new check "a climber round its trunk": 160 trunks round landings in forests on 4 worlds with climbers, each
  climber posed 1 m and 2.5 m up each trunk (320 poses, 1,280 feet). Its feet off the drawn bark, as it clung before: 12.8 cm on the mean,
  43.9 cm at most; now 0.7 cm and 3.5 cm. Four of a herd onto each trunk from one side: two on one side within a body's length 770 times as
  they chose before, 0 now.
- `bestiary ways`: `ways_climber.png` shows the octopod's legs on its thin trunk (before they stood in the air beside it).
- The tree drawing reads the moved formula bit for bit: every other `bestiary` frame and `regress`'s 24 hashes unchanged.
