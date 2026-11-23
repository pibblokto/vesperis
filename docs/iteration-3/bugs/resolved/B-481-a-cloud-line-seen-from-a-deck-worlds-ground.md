# B-481: a cloud line seen from a deck world's ground

status: resolved 2026-11-23 (J-16)
severity: S4
area: surface/surface_palette.cpp (the sky bank), surface/surface_sky.cpp (`drawSky`: the deck's night light)

## Report

KI-508, found in J-14 (2026-11-23): "On a venusian world's ground the sky shows nothing of a cloud line: its dyed streaks and its glow are
drawn from orbit alone (they lie on the deck's top)."

## Cause

A world under an opaque deck (a venusian world's clouds, a hydrocarbon world's haze) draws its whole sky as the deck's underside, in the
sky bank with the world's own colours. A cloud line's dye and glow were drawn only into the cloud bank of the ground's cloud layer, and a
deck world has no cloud layer: from the ground, nothing of the line showed.

## Fix

- By day, the deck's underside takes the line's dye: the sky bank's whole ramp moves toward the dye's colour by the clouds' tint, up to its
  brightest stops, since a deck's bright sky lies there and a deck shows no sun's disc.
- At night, a glowing line's light shows on the deck's underside in its form (patches, veins or a whole deck), where each ray meets the
  deck 12 km up, drifting with the wind. It is drawn in the made clouds' bank, on a grid as the clouds' glow is.

## Verification (2026-11-23)

A new check in the `seeding` group, the venusian world of the forty under the test layer's red, glowing clouds (as J-14's globe check):

- On the old code: FAIL. By day the sky was 249 208 161 with the layer and as generated alike; at night 65 60 57 alike, with no pixel in
  the made clouds' bank.
- On the new code, by day (the sun 0.58 up): 232 160 133, redder than the world's own 249 208 161 (the red share up 18%, the green down
  23%). At night (the sun 0.76 under the horizon): 63,264 of 256,000 sky pixels glow in the made clouds' bank, at 39 111 103 (the glow's
  sea green), and the sky's mean is 58 72 69, against the world's own 65 60 57.
