---
id: R-001
title: planets and moons visible from the surface more often (but not too often)
status: done 2026-09-26 (M5-09; part A and part B shipped, targets partly met)
requested: 2026-09-26
related: M5 (astronomy), M9-01 (survey tool), M9-12 (system architecture), B-005 (dim stars), M1-10
---

## What
More landings where another world hangs in the sky as a disc, like the crescent of Broolyuck V seen from its icy moon Broolyuck V-A (screenshot below). It should stay a real consequence of the system geometry, not a cheat, and it should not happen every time.

## Why
Those moments are the best thing exploration produces; they are what you remember and screenshot. Right now they feel rare and accidental.

## Must / should / could
- must: whatever appears in the sky is where the maths puts it (position, size, phase).
- should: a body disc in the sky on noticeably more landings than now; still plenty of empty skies.
- could: the landing map tells you before you land whether something big will be up.

## References
User screenshots, 2026-09-26: from the surface of BROOLYUCK V-A (59.9 N 23.7 W, EPOC 6011:003.600, an icy moon, gravity 0.07 g) the parent planet BROOLYUCK V fills a quarter of the view as a crescent sitting on the horizon; from orbit around V-A (1982 km) the moon and the half-lit parent share the view. The original Noctis IV produced the same kind of scene from moons of large planets.

## My notes (filled by Claude)

### What the numbers say today
Throwaway survey over 2,646 systems near the start region (28,484 landable bodies: 36% planets, 64% moons, of which 39% are moons of gas giants), sampling random landing sites and times:

| Situation at a random site and time | Share of sites |
| --- | --- |
| a body with angular radius above 0.5 deg is above the horizon | 41% |
| above 1 deg (a clear disc, about 8 px across at 320x200) | 37% (41% on moons, 31% on planets) |
| above 3 deg (a big disc) | 28% |
| above 10 deg (fills the view like the screenshot) | 10% |
| another planet is above the horizon, but only as a point | 77% |

When a disc above 0.5 deg exists, it is the parent planet 73% of the time, the body's own moon 26%, a sister moon under 1%, another planet almost never (the largest another planet ever gets is 1.2 deg; typically 0.01-0.1 deg, far below one pixel, which is 0.25 deg).

So the geometry already puts a disc in the sky on more than a third of landings; the reasons it feels rare are visibility and site choice, not orbits:

1. **Points vanish.** Bodies smaller than 1.2 px are drawn as a single pixel of intensity 18-55 before the mush filter, which cuts a lone pixel to a quarter of its value (same cause as B-005 for stars). Other planets are therefore practically invisible, though one is above the horizon 77% of the time.
2. **Day skies hide bodies.** Bodies are drawn into the sky bank and only where they are brighter than the sky pixel, so on atmosphere worlds a moon or a parent by day mostly loses against the bright sky; the default landing site (the sub-ship point) is on the lit side, exactly where the sky is brightest.
3. **Locked moons are landed on blindly.** 70% of moons are tidally locked, so the parent is visible from one hemisphere only; the default site and the random site (`R`) ignore where the parent is.
4. **Sister moons are rare and small** (under 1% of sites).

### Proposal (two parts, both consistent with the maths)

**A. Show what is already there** (size M, with B-005):
- Planets and small moons as bright "wandering stars": a 2x2 point plus halo that survives the mush, brightness from phase, albedo and distance (an evening-star look), with a name on the data sheet.
- Daytime visibility: a disc above 0.5 deg is drawn through the day sky when its lit surface is brighter than the sky (a daytime moon), using the sky-bank ramp so it still looks Noctis.
- Landing map readout "IN THE SKY: BROOLYUCK V, 34 DEG UP, HALF LIT" for the biggest body visible from the cursor, and a key that jumps the cursor to the point where that body is highest (the sub-parent point on a locked moon).
- Surface HUD: bodies listed with altitude in the data sheet (`F2`/`I`).

**B. Shift the odds in generation** (size M, needs the survey as a `vesperis_test survey` mode, M9-01):
- Double planets: about 7% of rocky, felisian, venusian and quartz planets get a companion of 0.55-0.95 of their radius at 5-9 radii, both locked to each other; from either surface the other is a 6-11 deg disc. Covers the "planet visible from a planet" case honestly (Pluto-Charon).
- Bigger first moons: allow the largest moon up to 0.5 of the planet radius (0.42 now) and place the first moon at 3.0-4.5 radii (3.5-6.5 now).
- Gas giant moon systems with the second and third moons closer together, so sister moons show as 0.5-2 deg discs more often.
- Targets, measured by the survey: a disc above 1 deg up at 45-50% of random sites (37% now), above 3 deg at 35% (28% now), above 10 deg at 12-15% (10% now). Anything beyond that makes it ordinary.

**Not proposed:** shrinking planetary orbits so that planets show discs from other planets; that would need orbits about 100x tighter and would break the scale of the systems and the sun sizes.

### Where it lands in the roadmap
Added as M5-09 in `ROADMAP.md`; part A can ship with the B-005 fix and the landing-map work (B-004), part B with M9-01/M9-12. Acceptance: `vesperis_test survey` prints the table above and the targets are met; `moonsky` and a new `surface` scene show a daytime moon and a companion planet.

### Outcome (2026-09-26, M5-09)

Part A: far bodies are drawn as scale+1 squares that survive the mush, brighter with phase and closeness and hidden by a brighter sky; daytime discs already drew through the sky (kept); the landing map has the `IN THE SKY` line and `J` jumps the cursor under the biggest body; the surface data sheet lists what is up. Part B: double planets (7%), first moons at 2.8-4.2 radii up to half the planet's radius (big half the time), giants' inner moons tighter. Survey over 7,116 random sites: a disc over 1 deg at 39% (target 45-50), over 3 deg 33% (35), over 10 deg 18% (12-15), a sister moon 8% of the discs (under 1% before); left there so the big view stays a treat (`10-decisions.md`).
