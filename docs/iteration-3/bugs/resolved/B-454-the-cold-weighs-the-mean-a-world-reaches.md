# B-454: the cold weighs the mean a world reaches

status: resolved 2026-11-22 (1.5.0, before its release)
severity: S4
area: life (`life/seeding.cpp`: `coldFelt`, `madeFaunaOf`)

## Report

KI-486, found in J-11 (2026-11-22): "The cold kills a world's animals by how much colder it is alone (from 4 C), so a hot world cooled into
the temperate loses them to the cold; the heat now weighs the mean a world reaches (J-11)." The user asked for it to be fixed (2026-11-22,
with the other known issues of the J series).

## Cause

The cold's harm was `smoothstep(4, 16, -warmC)`, by the drop alone, where J-11 had made the heat's harm weigh the mean a world reaches.

## Fix

The cold's harm is weighed by how cold the world now is too (`coldFelt`): none while its mean stays over 15 C, all of it from 5 C, the
mirror of the heat's. A hot world cooled into the temperate keeps its animals; a cold one cooled loses them as before. The plants have no
cold harm (a frost's burn is a line's look).

## Verification (2026-11-22)

- The unit's `seeding` group, new check "the cold harms by the mean reached": the cold haze lines of `seed climate`'s twenty worlds that
  cooled a world 4 C or more. Before: 1 of 2 warm worlds cooled into the temperate or over spared the cold. After: 2 of 2; 3 of 3 cooled
  to 5 C or under still lose animals to it.
- `seed kinds`: four cold hazes on warm worlds change only the looks now (COLD HAZE, SEA SMOKE), where they harmed.
