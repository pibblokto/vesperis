# B-410: a planet approached from a belt pulls the ship back to the belt

status: resolved 2026-10-07
severity: S2
area: the Stardrifter's fine approach and parking (O3's belts)

## Report

The user, while X-04 was being built: "if you approach a Belt, then select a planet and try to approach it suddenly you
are getting pulled (stardrifter is getting pulled) back to belt though tab button shows you are next to planet you
selected".

## Cause

`Game::startApproach` (`game/autopilot.cpp`) set the local target and cleared the body's parking, but kept the belt's:
`ship.parkedBelt` (the belt the ship was parked in) and `ship.targetBelt` (the belt as the local target). The approach
from the system analyzer flew to the planet (the analyzer's pick clears the belt target), but at its end the ship parked
with both a body and a belt. In `updateShipMotion` the parking's branch for a belt comes first, so from the next frame
on the ship was put back beside its rock every frame, while the analyzer and the local target named the planet. Every
later approach in the system ended the same way (the capsule refused there too: `NO WORLD UNDER THE SHIP`), and a second
approach to the planet said `ALREADY IN ORBIT`: only another star (the arrival clears both) or a load got out of it.

The flight computer's `LOCAL TARGET: NEXT BODY` set the body and kept the belt targeted, so its `FINE APPROACH` flew back
to the rock (an approach goes to the belt whenever one is targeted).

O3 (R-302) added the belt's parking; its flow step flew from a planet's parking into a belt, never back out.

## Fix

* **A body's approach leaves the belt**: `startApproach` clears `targetBelt` and `parkedBelt`, as `startApproachBelt` clears
  the body's.
* **The flight computer's next body** clears the belt target, as the space view's `L`, the click and the analyzer do.
* **A save made in that state** (parked at a body and in a belt) loads parked at the body its approach reached.

## Verified

The unit's new check (`a planet approached from a belt keeps its parking`, 193 in all): parked beside a rock of
Lathyari's first belt (150 0 23), the analyzer's planet (Tab, Up to it, Enter) parks the ship 3.5 radii from Lathyari I
and holds it there, where it was 4,453 radii away beside the rock; the flight computer's next body and approach do the
same, where it flew back to the rock. A save written in the bad state (scratch) loads 3.5 radii from the planet.
`unit` 0 failures, the regress unchanged (22 hashes), `flow` exit 0 (its belt step parks in the belt as before). Nothing
drawn changes, so `bench check` was left for X-04's run.
