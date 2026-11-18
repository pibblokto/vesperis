# B-438: a world's season was kept by another system's body

status: resolved 2026-11-18 (KI-442, before the 1.5.0 release)
severity: S4
area: the space view's season (`space/space_view.cpp`: `setSeason`, `genFor`, `mapIfReady`; `space/space_globe.cpp`: the telescope's
plate)

## Report

The user, 2026-11-18: "fix 441 and 442 please". KI-442, found by B-436: "The space view keeps a world's season step by index, not by
system: another system's body of that index decides when its map is made again, so a map can keep a season a step off (`space`,
Wyariothmai I)."

## Cause

The space view makes a world's map again only when its season moves a step (eight an orbit, `setSeason` each frame for the system
drawn). The step and the season the world's generator is made at were kept in two arrays by the body's index (`seasonBucket[64]`,
`curSeason[64]`), and every system has a body 0, 1, 2:
- drawing another system set each index's step to its own body's, and dropped only that body's generator;
- back at the first system, a world whose own step had moved to the one the other system's body left there looked unchanged, so its
  generator kept the season it was first made at, and its next map was made with it: a step off.

B-436 found it in z07's `space` sweep: Wyariothmai I's generator, made at t 1234 at season -0.04, was kept through t 5000 and 7000,
where its own was +0.06 and +0.11. Its globe and the telescope's plates drew another season's snow and clouds. In play the same came
of a jump away and back: a world could keep its first visit's season on its globe and its landing map, while the ground, which takes
its own season at the landing, did not.

Two smaller faults went with it: a body past index 63 took body 0's season, and the 0.3 drift test could never fire (a step is 0.25
wide).

## Fix

The step and the season are kept by the body's seed (`SpaceRenderer::seasons`), as every other cache of the space view is
(`rules-rendering-frame.md`).
- `setSeason` sets each world's own step; a new step drops that world's generator.
- A world whose season was never set has step -1 and its generator made at its mean (0, as a new renderer's was).
- `seasonStepOf` gives the step the map and the telescope's plate are made for.

## Verification (2026-11-18)

**A unit check** in the `basics` group: "a world's season is its own whatever system was drawn between (B-438)". Twelve worlds of six
systems near home, each drawn at t1; another system whose body of its index stood in the world's step drawn at t2; the world again
at t2.
- With the fix: 12 of 12 generators at their own step's season.
- With the index (a scratch build of this change undone): 0 of 12, e.g. Thapheaelio II at +0.20 for its own -0.08, Aithen VI at
  -0.01 for its own +0.25.

**The harness had the same fault in its own frames.** The space sweep warmed a world's map, then drew the frame, which set the
season (`warmMaps`, then `render`):
- by the index, the warmed map survived only where another body's step stood at the index; otherwise the frame made it again on its
  thread and drew the globe flat: 20 of the sweep's 60 globes in b436 (TECTONIC, HOTHOUSE, ICY, WANDERER among them);
- by the seed it never survived, since a world never set has no step.

So `warmMaps` takes the frame's time and sets the season first, as the harness's rule says (`rules-harness.md`), and the four places
that warmed a map by `mapFor` before a frame do too. Every globe of the sweep now has its map, at its own season.

**The battery** against b436 (`regress` unmoved, 24 hashes):
- `sweeps`: the globes; the telescope's frames of Wyariothmai I (`b438_telescope_ground_before_after.png`: its ground's snow at its
  own season); four landing maps; the consistency sheets (map, globe and ground at one point): 14 of 51 types over the threshold,
  22 before. The globe came nearer the landing map in 15 of the 24 sheets that moved (TERMINATOR 157 to 26, SNOWBALL 185 to 27,
  WANDERER 127 to 10) and went farther in six: the two cloud decks, which their verdict leaves out, STEPPE by 27, TUNDRA by 12,
  TECTONIC by 6 and AMMONIA by 2.
- `flow`: ten frames and a line, all of them B-438's (with it undone the flow is b436's). Its landing site's world is mapped at its
  own season after the expedition's other systems: the landing map, its zoom and sky, the drones' screens, the roads' zoom, the
  almanac's site. The almanac there lists 5 events where it listed 9: one front in five days where there were three.

**The frames:** `shots/tests/b438_space_globes_before_after.png`, four of the sweep's globes that b436 drew flat (above) and their
maps now.

Nothing stays open.
