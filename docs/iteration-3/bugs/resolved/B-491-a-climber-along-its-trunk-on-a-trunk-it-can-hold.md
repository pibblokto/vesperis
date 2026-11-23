# B-491: a climber along its trunk, on a trunk it can hold

status: resolved 2026-11-23 (J-16)
severity: S4
area: surface (`surface/animal_mesh.cpp`: `turnPose`, `AnimalPoseIn::axis`; `surface/creatures.cpp`: `climbPose`, `climbTreeNear`)

## Report

KI-496, found in B-468 (2026-11-22): "A climber on a trunk thinner than its body grips within the girth's sides with its body straight
up the trunk: it hugs a pole, and it never tilts with a leaning trunk." The user asked for every known issue of 1.5.0 to be handled
(2026-11-23).

## Cause

- The pose was turned onto the trunk with its forward always straight up (`turnPose`), so on a leaning trunk the body stood upright
  beside the bark while its feet followed the lean.
- A climber took the nearest live tree within 25 m (`treeNear`), however thin. On 28% of the trunks round the test landings (67 of 239)
  the trunk at the height it climbs to was thinner than the climber's body was wide, so its feet, held within the girth's sides,
  gathered under it round what looked like a pole.

## Fix

- The pose takes the trunk's axis where it clings (`AnimalPoseIn::axis`, from 0.2 m under the ground to the top, leaning with it), and
  `turnPose` lays the body along it, with its belly to the bark square to the axis. `climbPose` sets the climb's inputs for the frame
  and the unit alike.
- A climber takes the nearest live tree within 25 m whose trunk as drawn is at least as thick as its body is wide, at the most it climbs
  to (`climbTreeNear`). With none near, it does what it does without a tree (it runs, or rests on the ground).

## Verification (2026-11-23)

- The unit's `animals` group, new check "a climber along its trunk's lean, on a trunk it can hold (B-491)": 105 leaning trunks (up to 6
  degrees) on 4 worlds, 210 poses. The body off the trunk's line: before 3.7 degrees on the mean and 5.7 at most; now 0.0 and 0.0. Its
  840 feet on the bark: 1.7 cm off at most (3.4 before). From 239 points round the landings, the trunk taken was thinner than the body
  67 times before and 0 times now; 24 times there was no trunk it could hold within 25 m.
- B-468's "a climber round its trunk" still passes (an upright trunk's pose is as before: 0.7 cm on the mean, 3.5 at most).
- `bestiary ways`: 15 of 15 ways drawn. `ways_climber.png` moved: the octopod of Yiofyarivin II, wider than its world's thin palms, now
  runs instead of hugging one, and the frame shows the next world's climber (Niofionaliosh I) lying along a thick trunk. The other 14
  ways' frames but the shallows' are the same bit for bit. `regress`: 24 hashes, none changed.
