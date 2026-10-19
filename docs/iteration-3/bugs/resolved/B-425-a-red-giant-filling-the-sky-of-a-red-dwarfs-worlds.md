# B-425: a red giant filling the sky of a red dwarf's worlds

status: resolved 2026-10-18 (1.3.2, the second of the patch's items)
severity: S2
area: the companion stars (`galaxy/system.cpp`, `GEN_VERSION` 14), the light of two suns on the ground (`surface/site.cpp`,
`surface/surface_view.cpp`, the landing map's day)

## Report

The user, with five screenshots of Yaiphuyiabrus (PARSIS -31900 +3 -35668, `S06+S03 MULTIPLE`): from orbit of a felisian world, from
the fine approach (the sun fills the frame) and from the ground at 3.9°N 99.4°W, EPOC 6011:003.601, a forest under a disc that takes
most of the sky (the ground was Yaiphuyiabrus IV's, its people's world: B-426's screenshots of the same visit stand in its capital): "I found a sun that was either just ridiculously huge or it's just that planets that are so close. I don't know,
it's weird, it should be this way, especially if it's a felisian planet, vegetation wouldn't survive it at all..."

## Cause

* **A far companion's orbit was drawn from the primary's widest orbit alone.** M5-01 put it at 2.5-7.5 times the widest planetary
  orbit, its eccentricity 0.05-0.45, without its own size or light: written for the yellow and orange stars, whose worlds reach
  10^8-10^9 km. S-01 brought the red dwarf (the commonest star, its worlds within about 1.2e7 km) and the giant varieties, and a red
  giant (3.5e7 km +-40%) drawn as a red dwarf's companion orbited 3-9e7 km out, about its own radius from the worlds. Yaiphuyiabrus:
  the giant (R 3.24e7 km, L 15.9) at 4.52e7 km, e 0.31, so its periapsis (3.11e7 km) lay inside its own radius; from the three
  felisian worlds it spanned 57 degrees at its farthest and swallowed them at its nearest; at the reported place and time (IV) it stood
  49 degrees up and 94.5 degrees across. Over a uniform sample of the disc (160,778 stars, 25,869 far companions) a far companion spanned more
  than 10 degrees of a world's sky in 1,277 systems (0.8% of the stars), more than 45 in 627, and reached into a world's orbit in 337.
* **Its light was never the worlds'.** The primary's worlds are generated before the companion and typed by the primary's light
  alone (`pickPlanetType` reads `equilibriumTemp(L, r)`): a companion that warmed them made their types wrong. In 8,213 of the far
  companions its light alone would have kept the primary's farthest world over 150 K; under the giant Yaiphuyiabrus's felisian
  worlds would have been near 390 K before the greenhouse.
* **A close companion was never asked whether it was bright.** 35% were close (3-6 summed radii, when inside 40% of the first orbit),
  the worlds circling both, whatever their light: a white dwarf's yellow companion spanned 20-28 degrees of the first world's sky (37
  systems of the sample over 20 degrees, 376 over 10) over worlds typed by the dwarf's luminosity of 0.02 that it would have scorched.
* **The light of a far sun on the ground was a near one's.** `SurfaceSite::sun2` gave the companion the perceptual floor of 0.35, so
  a far one lit like a dim sun, and `computeEnvironment` added the primary's whole light to the companion's once the primary had set
  (the factor `lf1 (1 - share) + (lf1 + lf2) share`, at share 1 `lf1 + lf2`): the reported spot, Yaiphuyiabrus IV at three in its
  night (a day of 2.4 hours; the dwarf 44 degrees down), read the sky 1.00 and the light 1.89 under the giant (49 degrees up, 95
  across), and with the giant moved out alone it would still have read 0.47 and 1.09, the night ground lit as a day under a dark sky
  (the night side of Yaiphuyiabrus III, locked to the dwarf, likewise: 1.00 and 2.11, then 0.37 and 1.31). On a world of a companion the far primary kept the 0.62 floor
  of a world's own sun and its full day sky: Phyaalion B I (150 0 134), an ocean world of a red dwarf companion, had a blue day at its
  own midnight under a blue giant 1.9e10 km away (the light 0.62 where its own is 0.076), and its clock was the primary's.

## Fix

* **The far companion keeps its distance** (`StarSystem::generate`). At its closest approach it stays far enough from the primary's
  farthest world (a planet's apoapsis and its widest moon's) that it spans 3 degrees of that world's sky at most (`COMPANION_DISC`)
  and its light alone would warm it to 100 K at most (`COMPANION_HEAT_K`, an equilibrium temperature: two or three kelvin on a
  temperate world); its own worlds keep within a third of that gap (the widest its up to three can reach from its first key) and see
  the primary 3 degrees wide at most. The draws are the same: only an orbit under the floor moves out, and its period with it.
* **A close companion is a dim one** (a tenth of the primary's light at most, `CLOSE_LIGHT`, and no wider than the primary in the first
  world's sky at its nearest); otherwise it is far, with worlds of its own. A black hole keeps its close companion (S-06's stream).
* **A far sun lights as much as it does.** The world's own sun keeps the floor (0.62) and the other its true factor
  (`starLightFactor(L, d, 0)`; `StarSystem::ofCompanion` says whose world it is); a sun under the horizon adds none to the blend; the
  blended light's glow in the sky follows its share (`env.glow`); on a companion's world the far primary's day sky, disc, glare and
  flare follow its light beside the world's own sun (`env.primaryRel`), the local time is the companion's (`sun2` has its hour angle),
  and the landing map's day and time are its own sun's.

## Verification (2026-10-18)

* `vesperis_test unit`, four new checks: over the home cube and a cube of the core (1,337 multiples) no far companion wider than 3.00
  degrees or warmer than 100 K at the primary's farthest world, the own worlds within 0.32 of the gap, no close one over 0.100 of its
  primary (the black holes' 17 apart); Yaiphuyiabrus keeps its worlds (venusian, three felisian, the third locked) and the giant, now
  4.5e9 km out, spans 1.20 degrees at its widest; the reported place and time, on IV (B-426 moved the check there from III's night
  side, which read 0.13 and 0.12), read the sky 0.17 and the light 0.12 under the giant (0.65 degrees across, 56 degrees up) with the
  dwarf 44 degrees down; Phyaalion B I at its midnight reads the sky 0.07, the light 0.08, the local time
  0.99.
* The sample before and after (scratch tools beside the harness): every primary's worlds the same in all 160,778 systems (type, orbit,
  size); the companion moved in 18,555 of the 32,907 multiples (the far orbit a median 2.4 times out, nine in ten under 9.6 times;
  3,383 close companions far now; the companion's worlds new or changed in 3,053 systems, 2,520 of them a close one's gone far); afterwards no far
  companion over 3 degrees (530 over 2) or 100 K, none reaching into a world's orbit.
* `regress` 24 of 24 unchanged; `flow` passes. One unit line moved: the site of Eliameavevas I-a, whose light
  blends a companion, finds its light at 40.1 degrees (39.8 before; 40 asked).
* The reported spot's frame at the reported time (IV), before and after: the white sky of the giant's disc; a night under a small red
  sun 56 degrees up. III's night side likewise (the giant 69 degrees up); its day side the dwarf's.
