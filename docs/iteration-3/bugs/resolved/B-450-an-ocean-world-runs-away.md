# B-450: an ocean world runs away

status: resolved 2026-11-22 (1.5.0, before its release)
severity: S4
area: life and galaxy (`life/seeding.cpp`: `madeAirOf`'s runaway; galaxy/planet_grounds.cpp: `driedSeaFloor`; `planet_grounds2.cpp`:
the dark ocean; surface/site.cpp: `hasWater`; galaxy/made.cpp: the made hothouse's words)

## Report

KI-458, found in J-04 (2026-11-21): "Only a living world with seas runs away: an ocean, a hycean or another wet kind warms past 42 C and
keeps its seas." The user asked for it to be fixed (2026-11-22, with the other known issues of the J series).

## Cause

The runaway was gated on a living ground with seas. Only the living ground could draw a made hothouse's dry seas (`deadCoverOf`).

## Fix

- Any world with seas runs away once its lines take its mean past 42 C (every kind with seas has water ones). One already over 42 C as
  generated keeps its seas, as the ice's run leaves a world already under its line: a dark ocean's under a deep air are what it is
  (the battery's `seed sea` found one at 47 C and 569 atmospheres running away at its first step before this was added).
- A made hothouse that was an ocean, a hycean or an eyeball lies on its old sea floor at its heights as generated: the salt its sea held
  crusting most of it, dust on the swell's rises; the floes, their shelf and the sea glacier go with the sea. A dark ocean's floor dries
  to salt in its deeps and dust. No sea is left on the ground (`hasWater`), and an exotic liquid does not flood it again.
- Its sentence names no forests where the world had none: "Its seas boiled away into a thick hot air, and salt lies white on the old sea
  floor."

## Verification (2026-11-22)

- The unit's `seeding` group, new check "an ocean world runs away": the four worlds of seas that are not living ones among `seed
  climate`'s twenty, each given three strong greenhouse lines (they lie at -15 to -33 C). Before: 0 of 4 dried. After: the ocean (at
  -15 C) a MADE HOTHOUSE, no wet point left on 648, its floor salt and dust; the three eyeballs stay under 42 C.
- `seed kinds`: two seedings of the forty worlds' hycean now run away (MADE HOTHOUSE), its CO2 and its methane line.
