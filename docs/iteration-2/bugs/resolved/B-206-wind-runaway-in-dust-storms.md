---
id: B-206
title: Absurd wind, flickering sun and radial streaks on a thin-atmosphere world
severity: S2
status: resolved 2026-09-27
reported: 2026-09-27
---

## Where
State / screen: surface, MUTHEYN II (thin atmosphere), 36.0N 114.3E, EPOC 6011:003.601, during a dust storm.
Screenshot: the user's (`WIND 79488080605566 KT E  DUST STORM 33%`, brown streaks radiating from the horizon across the sky).

## Steps
1. Land on a thin-atmosphere world while a dust storm is on (`DUST STORM` on the HUD).
2. Watch the wind readout for a few seconds.

## Expected
A storm wind of a few tens of knots; a steady sun; dust streaming sideways.

## Actual
The wind readout climbs to 1e13 knots within two seconds, the sun and the cloud shadows flicker, the dust particles become lines from the horizon to the camera, the readout runs into the stamina bar.

## Cause
`computeEnvironment` runs every frame; on thin-atmosphere worlds it multiplied `env.windKnots` by `1 + 2.5 dust` each time, while the wind itself is only reset every two seconds: 3.5 to the power of 120 frames. Every consumer scales with the wind: the cloud drift (`wind x 0.5 x t`, so the sky clouds and the ground shadows sampled a different place every frame: the flicker) and the dust particles' drift (the streaks). Separately, the drift formula `wind x 0.5 x t` jumped by tens of kilometres at every two-second wind change even with a sane wind (t is 3.6e6 s), which moved the clouds in front of the sun at each update.

## Fix
The storm factor is part of the two-second wind update (`base x gusts x rain x (1 + 2.5 dust)`), never compounded; the cloud drift is an integral of the wind over time (`windDriftX/Z`, 0.5 m per knot-second, seeded to the old formula's value on the first frame so the regression frames are unchanged); the HUD says `DUST 33%`. Check: `unit` "storm wind under 200 knots" (a thin-atmosphere site in a storm, 300 frames: worst 46 knots).
