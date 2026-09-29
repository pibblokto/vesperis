---
id: B-307
title: The buggy hops off every downhill instead of driving it (a "ladder" descent)
severity: S2
status: resolved 2026-09-27
reported: 2026-09-27
---

## Where
State / screen: surface, driving
Star / body shown in the HUD: any world with slopes (reported after the O6-02 relief on felisian mountains)
EPOC shown in the HUD: n/a
Save attached: no

## Steps
1. Land on a hilly or mountainous world, unfold the buggy (`B`), get in (`E`).
2. Drive down any steady slope of more than about 15% at more than 10 m/s.

## Expected
The buggy rolls down the slope on its wheels, gaining speed.

## Actual
It "jumps off" again and again: a short flight, a thump, a loss of speed, another flight. The descent becomes a
ladder and is slower than the climb.

## Notes
Frequency: always on a steady descent above 10 m/s.

Cause (`updateBuggy`, `surface/buggy.cpp`): the buggy left the ground whenever the ground contact dropped faster
than 3 m/s under it, which a 30% grade does above 10 m/s, and it launched horizontally (`vy = max(0, vy)`). It then
fell back onto the slope, landed with a thump that cost 15% of its speed, and launched again.

Fix: the wheels leave the ground only when the ground falls away faster than gravity and the suspension can follow:
the vertical acceleration the ground asks for, `(vyGround - vy) / dt`, beyond four g and at least 40 m/s^2, at
speed. A steady descent asks for none. A launch keeps the current vertical speed (a crest throws it up, a lip lets it
drop), and a landing is judged by the speed relative to the slope (`Buggy::groundY` keeps the previous contact
height), so touching down on a descent costs nothing. `vesperis_test descent` drives the pinned felisian mountain
site's steepest 200 m run (grade 0.45) at full throttle: 0% of the frames airborne, no landings, 25 m/s on average.
