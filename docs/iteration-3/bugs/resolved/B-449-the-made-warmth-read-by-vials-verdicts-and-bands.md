# B-449: the made warmth read by vials, verdicts and bands

status: resolved 2026-11-22 (1.5.0, before its release)
severity: S4
area: life and surface (`life/seeding.cpp`: `seedingRun`, `seedVerdictOf`, `seedPlaceOf`, `seedHabitatOf`; galaxy/planetmap.cpp:
`madeMeanAt`; surface/sampling.cpp: `placeMeanC`; game/game.cpp: `seedingsAt`)

## Report

KI-457, found in J-04 (2026-11-21): "Samples, a later line's verdict and the spreads' bands read a seeded world's warmth as generated: a
vial from a world 20 C warmer is read at its old warmth, and a second line is judged by the old climate." The user asked for it to be
fixed (2026-11-22, with the other known issues of the J series).

## Cause

The HUD's TEMP read the made warmth, but a vial's microhabitat (`SurfaceView::placeMeanC`), the band a spread lives in (`madeMeanAt`) and
a line's verdict and share (`seedVerdictOf`, `seedHabitatOf`, over the world as generated) did not. Each seeding was run alone.

## Fix

- A vial's mean warmth takes the made warmth where the world has air, as TEMP does.
- A spread's band is read at the made warmth (`madeMeanAt`).
- The game runs a world's seedings in order, each after the ones before it (`seedingRun`'s `earlier`): a line's place, its medium, the
  natives at it and its share are read on the world as those lines had made it when it went down (its warmth, its covers, its frozen or
  dried seas). It runs one way, the earlier lines on the later, so nothing loops. The world's mean as generated stays the line the runaway
  and the ice pass (`SeedVerdict::worldC`).

## Verification (2026-11-22)

- The unit's `seeding` group, new check "a later line read at the warmth made": fourteen CO2 lines that warmed their worlds 3 C or more,
  a second line let go at the place once each had. Before: 0 of 14 seas read at the warmth made (a tundra's sea 18.1 C both times, its
  first line having made it 13.2 C warmer), 0 bands read at it. After: 14 of 14 (an archean's sea 25.8 C, 40.0 C once its first made it
  14.2 C warmer), 14 bands.
- The battery's `seed traits`: the coloured clouds' line warmed its hothouse 9 C (30 to 40 C), and its red waters now end at its own
  limit of warmth, the tropics' seas left their own colour. With the made warmth taken out of a line's band for a test, the seas are red
  to the equator, as before.
