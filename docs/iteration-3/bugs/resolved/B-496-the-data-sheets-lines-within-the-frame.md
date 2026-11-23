# B-496: the data sheet's lines within the frame

status: resolved 2026-11-23 (J-16)
severity: S4
area: game (game/data_sheet.cpp: `renderDataSheet`, the body's sheet and the belt's)

## Report

KI-440, found in Z-07 (2026-11-17): "The data sheet's GRAVITY line runs past the frame's right edge on most worlds (`EQUILIBRIUM 154 K
(-119 C)` cut after `(-`: 57-58 characters of the 52 a line holds); z06's `datasheet` frames show it too."

## Cause

`renderDataSheet` put each row of numbers on one line, whatever its length. The frame holds 50 characters with the sheet's margins
(its sentence wraps at 50), and several rows ran past it:

- GRAVITY with EQUILIBRIUM: 57 to 63 characters ("GRAVITY     26.21 G (256.8 M/S2)   EQUILIBRIUM 5000 K (+4727 C)", a second
  sun's sheet). Every one of the 100 bodies' sheets measured had a line over 50.
- RADIUS with MASS and TILT: 52 on an Earth-sized world, 56 on a giant.
- PERIOD with a retrograde ROTATION: up to 54. MAGNETIC with "A FAINT AURORA IN A GREAT STORM": 54.
- On the ground, SEASON: up to 79 ("SEASON      DECL +13.3  DAY 1.1 H OF 1.8  EQUINOX IN 106 D  SOLSTICE IN 404 D").
- A belt's PERIOD at its two edges: 57. Its ROCKS: 56 ("ABOUT 1303767348945.5 MILLION").
- The SEEDED, MICROBES, FORECAST and STAR lines could pass 50 in rarer cases, and the LIFE line wrapped at 52.

A world with no tilt also drew its SEASON line straight onto the frame (`line`, not one of the sheet's lines): over the world's
name at the sheet's top.

## Fix

- A line holds 50 characters. `fit` joins a line's parts while they fit and carries the rest under the label's column. `wrapped`
  wraps a long value under its label.
- The numbers: RADIUS with MASS; GRAVITY with TILT; EQUILIBRIUM (or a drifter's OWN HEAT) on a line of its own; PERIOD with
  ROTATION, the rotation carried to the next line where it does not fit.
- On the ground: SEASON's equinox and solstice under its declination and day. FORECAST, IN THE SKY, HERE and the made AIR are
  wrapped. The second sun's name is cut at 18 characters. LIFE wraps at 50.
- The SEEDED line carries its time and depth to the next line where its name leaves no room. The star's class is carried where a
  long star name leaves none.
- The belt's sheet: its period at the inner edge and at the outer on two lines; its rocks counted in the largest word that holds
  them, THOUSAND to TRILLION.
- The no-tilt SEASON line is one of the sheet's lines, paged with them.
- The world's name at the sheet's top is cut at 24 characters, clear of the page's number and the clock (a name can run to 28).

## Verification (2026-11-23)

- The unit's `states` group, new check "the data sheet's lines within the frame": the sheet of every body and belt of the scan
  region's first twelve systems in space, over all its pages, and the sheets of three landable worlds with a tilt on their ground.
  - By the old code: 100 of 100 bodies' sheets had a line over 50 characters, the longest 63; 3 of 3 on the ground, the longest 78
    (SEASON).
  - With the fix: 100 bodies' sheets and 6 belts' sheets, none over 50 (the longest exactly 50, a TRAITS line); 3 on the ground,
    none over.
- `datasheet 3`: none of the three sheets' lines past the footer. The frames: `shots/tests/datasheet_*_space.png` and
  `_ground.png`, with the second pages `_space2.png` and `_ground2.png`.
