# B-498: a planet of a second sun called a moon

status: resolved 2026-11-23 (J-16)
severity: S4
area: game (game/names.cpp: `bodyIsMoon`; data_sheet.cpp, telescope.cpp, guide_screens.cpp); galaxy/describe.cpp: `describeBody`

## Report

KI-453, found in R-446 (2026-11-20): "The data sheet calls a planet of a system's second sun a moon: its kind line reads "- MOON OF"
and the second sun's name, since any parent does (`renderDataSheet`)."

## Cause

A second sun is a body of its system, and its planets have it as their parent. Every screen took any body with a parent for a moon:

- The data sheet's kind line: "FELISIAN - MOON OF UNKNOWN".
- Its sentence (`describeBody`): ", a moon".
- The telescope's reticle line: "FELISIAN MOON".
- N through the telescope: the prompt "NAME THIS MOON", then the status "THE MOON IS NOW CALLED ROSY".
- The second sun's own sheet counted its planets as MOONS (its `moonCount`).

The analyzer's ORBIT line already said "FROM 2ND SUN", and the target's label "of the companion".

## Fix

- `Game::bodyIsMoon`: a body whose parent is a planet. A planet of the second sun is a world.
- The screens above ask it. The kind line reads "FELISIAN - PLANET OF THE 2ND SUN".
- The sentence says ", circling the second sun" (`describeBody`'s new `ofSecondSun`, which the data sheet passes; the harness's
  callers and the fingerprint keep the words as they were).
- The second sun's sheet counts its PLANETS.

## Verification (2026-11-23)

- The unit's `states` group, new check "a planet of a second sun is a world": the scan region's first planet of a second sun (in
  its sixth system) and a planet's moon, each read on its sheet, through the telescope and named through it.
  - By the old code: "FELISIAN - MOON OF UNKNOWN", the telescope "UNKNOWN  FELISIAN MOON", "NAME THIS MOON", "THE MOON IS NOW
    CALLED ROSY"; its sun "MOONS 1".
  - With the fix: "FELISIAN - PLANET OF THE 2ND SUN", no ", A MOON" in its sentence, the telescope "UNKNOWN  FELISIAN", "NAME
    THIS WORLD", "THE WORLD IS NOW CALLED ROSY"; its sun "ATMOSPHERE  NONE   PLANETS 1   LANDABLE NO".
  - A moon is still a moon: "BOMBARDED - MOON OF UNKNOWN" with ", a moon" in its sentence, "BOMBARDED MOON", "NAME THIS MOON",
    "THE MOON IS NOW CALLED ROSY".
- Auto-explore still treats a second sun's planet as a moon in how it weighs where to go (`autoexplore_space.cpp`); that is how it
  chooses, not what it says, and is left as it is.
