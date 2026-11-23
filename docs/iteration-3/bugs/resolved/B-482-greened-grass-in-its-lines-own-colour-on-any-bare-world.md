# B-482: greened grass in its line's own colour, on any bare world

status: resolved 2026-11-23 (J-16)
severity: S4
area: galaxy/made.cpp (`madeGreens`), life/seeding.cpp (a greening line's colour), surface/surface_terrain.cpp and flora.cpp, space/space_globe.cpp,
game/landing_map_build.cpp, drone_draw.cpp and sector_map.cpp (the greened grass's bank and ramp)

## Report

KI-509, found in J-15 (2026-11-23): "A desert greened by two lines of different colours shows the latest's colour in both on the ground,
the globe and the maps: its grass and shrubs take the grass bank, one a world."

KI-510, found in J-15 (2026-11-23): "Only the desert greens: the scorched felisian lends its grass bank to its clay, and other bare worlds
(rust, salt, thin air) keep a carpet or a lichen as an overgrowth."

## Cause

J-15 drew a carpet's grass in the world's grass bank, whose ramp `madeMatColour` set to the latest greening line's colour. One bank holds
one colour, so two lines' grass showed in one colour. Because the bank was the world's own, only a world whose grass bank was free could
green: `madeGreens` asked for a dry world with no plants and no accent in that bank, which left the desert alone.

## Fix

- Greened grass is now drawn in its own line's overgrowth bank, as the line's cover already was (B-457's rule: a colour a line). That holds
  on the ground (`materialLook`, the shrubs and tufts), on the globe (the line's stain bank), and on the landing, drone and sector maps (the
  line's own grass ramp, a greened texel read at it). Two lines that green one world each show their own colour where their grass lies.
- A greening line's carpet takes its grass's colour (`MadeGrowth::colour` is its plants'), so one bank serves both.
- The world's grass bank is no longer used, so any bare world with air can green: one that is landable, with air, no plants and no seas of
  its own (`madeGreens`). That covers a desert, a rust, a salt, a thin-aired world, a scorched felisian and the rest. It is a GREENED DESERT
  once half its land is green, as a desert is.
- `seed kinds`' desert recipe keeps to a desert (J-15's world), now that other worlds green too.

## Verification (2026-11-23)

A new check in the `seeding` group. A red light-feeder's carpet, tuned for the desert's far side, is let go there with J-15's green one:

- On the old code: FAIL. At the first line's place, 0 pixels of the ground were in its own line's bank: both lines' grass was in the grass
  bank, in one colour.
- On the new code, at their 24th step, each line's grass and shrubs are in its own bank in its own colour. At the green line's place:
  562,761 pixels in bank 24, lit 0.35 0.47 0.15 (its plants 0.35 0.62 0.22). At the red line's place: 494,691 pixels in bank 28, lit 0.74
  0.13 0.10 (its plants 0.75 0.18 0.15). Where both lines lie, the later line's grass shows, as with any two lines' looks.
- Of the forty worlds, 16 are bare with air and are not deserts. The green line tuned for each place takes on two: a frozen felisian, all
  ice and snow, has no land to green, and a scorched felisian greens 42% of its land. The others refuse the line: too cold, or the
  ultraviolet.
- J-15's three checks pass on the new code. Its ground check now counts the line's bank: 562,761 pixels, lit in the pigment's green.
