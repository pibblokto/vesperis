# B-477: a full flask is fed

status: resolved 2026-11-23 (J-16)
severity: S4
area: life (life/cultures.cpp: `stateNow`)

## Report

KI-504, found in R-449 (2026-11-22): "A full flask, fed for good since R-449, still counts as starving for a WHEN STARVING switch (its
growth has stopped): a glow switched so lights at its crowd, a rest switched so never rests there."

## Cause

A switched line's culture reads its state from its numbers (`stateNow`). It counted the line as starving where the line as written
would not grow in its broth, and also once its crowd had used the broth up (`spent`, or 90% of its crowd). Before R-449 that was true: a
full flask held a while and then starved. Since R-449 the incubator feeds a full flask for good, and it stays FULL until its medium
changes, but the switch still read its crowd as starving. A glow switched WHEN STARVING lit at its crowd; a motion switched so swam in
the drop of a full flask; a rest switched so (a rest's own switch) was on in a flask that never starves.

## Fix

What WHEN STARVING means in a full flask: off. A full flask is fed. A culture is starving only where the line as written cannot grow in
its broth: nothing it eats, no light for a light-feeder, or a medium that kills it (the cold and the ice aside, which it waits out). This
is the condition rules-life-genes.md already states ("starving where the line as written cannot grow"); the code now reads only that.
In a broth that feeds it nothing, its WHEN STARVING genes are on while it holds (STARVING, a few minutes), and then it rests or dies, as
R-449 says. Nothing else reads the starving state: the seeding does not read switches.

## Verification (2026-11-23)

- The unit's `genes` group: two Y-04 checks now test this, and both failed on the old code.
  - A luminous heterotroph's glow WHEN STARVING: by the old code GLOWING at its crowd (state 12, crowded and starving). Now dark as it
    grows (its species aglow there), dark at its crowd (state 4, crowded), and glowing in a broth with nothing it eats (STARVING, state 8).
  - A swimmer's motion switched WHEN STARVING by S, written into a flask: by the old code its drop swam at its crowd (state 14). Now its
    drop is still as it grows (state 2) and at its crowd (state 6), and swims once its broth is changed to one with nothing it eats
    (state 14).
- The `genes` (27), `lab` (18), `incubator` (5), `experiments` (5) and `world` (3) groups: 0 failures. `lab cultures`' frames and
  printout are the same as the old code's (its switched lines are IN THE DARK and IN THE LIGHT).
