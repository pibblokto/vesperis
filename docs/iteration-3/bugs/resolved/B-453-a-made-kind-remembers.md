# B-453: a made kind remembers

status: resolved 2026-11-22 (1.5.0, before its release)
severity: S4
area: life (`life/seeding.cpp`: `stepsPast`, `madeAirOf`)

## Report

KI-485, found in J-11 (2026-11-22): "A made kind is read at the clock with no memory: a later line that warms a snowball past its line
thaws it at once, and a hothouse's seas come back." The user asked for it to be fixed (2026-11-22, with the other known issues of the J
series).

## Cause

The runaway and the ice's run were tested at the clock on the gases' warmth alone. A later CO2 line that lifted a snowball's gases over
-6 C thawed it in one step; a later cold haze that took a hothouse's gases under 42 C brought its seas back.

## Fix

The world's mean is walked over every line's step boundaries in order (`stepsPast`), carrying what it passed:

- A runaway goes on from the first boundary its mean passed 42 C, whatever comes after: its steam keeps it, and seas boiled off are gone.
- The ice runs from the first boundary its mean fell under -6 C and thaws only at one where the gases warm the world past the ice's own
  white too (-6 C and its cooling so far, up to 20 C).

It is still a pure function of the seedings and the clock. A world of one line, or of lines that only push one way, runs as before.

## Verification (2026-11-22)

- The unit's `seeding` group, new check "a made kind remembers": a tundra's snowball warmed by fifteen CO2 lines of rising rates, and a
  hothouse kind's made hothouse cooled by fifteen cold hazes. Before: 0 still frozen with their gases past -6 C, 13 thawed under the
  ice's own line; 0 still dry with their gases under 42 C, 12 with their seas back. After: 12 frozen, 0 thawed under the line; 12 dry, 0
  with their seas back.
- J-11's check "the ice's run" as before: FREEZING OVER from the clock's step 10, frozen at 16.
