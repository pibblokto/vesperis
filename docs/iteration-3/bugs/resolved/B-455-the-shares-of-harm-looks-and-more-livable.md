# B-455: the shares of harm, looks and more livable

status: resolved 2026-11-22 (1.5.0, before its release)
severity: S4
area: life (`life/seeding.cpp`: `madeAirOf`'s silver clouds, `madeKindOf`'s greening, `madeLivable`); surface (`surface_sky.cpp`: the
silver clouds)

## Report

KI-483, found in J-11 (2026-11-22): "Over `seed kinds`' six lines 50% of seedings harm, 20% change only the looks, 11% make a world more
livable (the design: half, a third, one in five); not tuned." The user asked for it to be fixed (2026-11-22, with the other known issues
of the J series).

## Cause

Three kinds of seeding showed nothing or less than they would: methane on an oxygen world (burned, it warmed a degree or two and was
named nothing), a cool world warmed a few degrees (only a world under 5 C made 3 C warmer greened), and a world made milder (a hot one
cooled, a cold one warmed) without a greening. A fifth of the seedings showed nothing.

## Fix

- Methane an oxygen air burns turns to water high up, frozen into silver clouds in the summer twilights of the high latitudes, as Earth's
  have spread with its methane: SILVER CLOUDS (`MA_SILVER_CLOUDS`), drawn on the ground as the noctilucent clouds' trait draws them.
- GREENED WORLD takes a cool world too: under 10 C as generated, made 2 C warmer, nothing harmed.
- More livable counts a world whose mean was moved 5 C or more toward 15 C, nothing harmed (`madeLivable`).
- The cold's harm by the mean reached (B-454) and the oceans' runaway (B-450) move some seedings between the shares too.

## Verification (2026-11-22)

- `seed kinds` (40 worlds, six lines, each at its 64th step). Before: 118 of 235 harm (50%), 48 change only the looks (20%), 25 make it more
  livable (11%), 44 show nothing (19%). After: 116 harm (49%), 44 change only the looks (19%), 44 make it more livable (19%), 31 show
  nothing (13%); 65 end as a made kind (MADE HOTHOUSE 6 for 4, GREENED WORLD 6 for 2), and the nine kinds' recipes each still draw.
- Tried and left: a line giving its whole gas from a twentieth of its world (`AIR_SHARE` 0.05 for 0.15) made 52% harm, 16% looks
  alone, 23% more livable and 9% nothing: more harm, no more looks.
- What stays (KI-498): most of the seedings that show nothing are sea lines on terminator worlds, whose seas are about a hundredth of
  their surface: a line there gives a twentieth of its gas and changes nothing it can be named by.
