# B-462: ruins taken wall by wall, in the cover's own look

status: resolved 2026-11-22 (1.5.0, before its release)
severity: S4
area: surface (`surface/overgrowth_ruins.cpp`: `pieceClimb`, `overgrowCell`, `growPieces`; `forms_draw.cpp`: `gatherSettlement`,
`drawFormItems`)

## Report

KI-477, found in J-10 (2026-11-22): "A building is taken as one: its cover climbs to one height, its middle's steps spread a sixth by its
hash, whatever its walls' place; taken whole, it shows as a darker mass." The user listed it with the others to fix (2026-11-22): "Now
let's handle all of these".

## Cause

`overgrowCell` read the steps since the cover's edge passed at each building's middle and gave the whole building one climb
(`RuinCell::climb`). Every piece of it, front or back wall, tower or porch, was covered to the same height. The steps since the edge
passed cannot tell the walls apart either: the edge moves kilometres a step, so the count is flat across a building. A face wholly under
the climb was drawn in the growth bank five shades under the ground's cover (`GROWN_DARK`), so a building taken whole read as a dark
block. Past 4 km the silhouettes ignored the climb, so a taken town stood bare again from far off.

## Fix

Each piece takes its own climb (`pieceClimb`): the building's, a third more on the building's side toward the way the cover came and a
third less on the far side (`RuinCell::towardX`, `towardZ`: toward the place of the line whose cover lies at the settlement's middle),
and a tenth either way by the piece's hash. Membranes, nodules and drapes follow their own wall's climb. A face wholly under its climb is
the cover alone, lit as the ground's cover and its structures are (B-466), no longer darker; the cover's edge across a face is two
shades under its wall. Past 4 km a silhouette is drawn in the cover's bank under its climb, a slice the climb crosses split there.

## Verification (2026-11-22)

- `overgrowth ruins` (the industrial test world's city and capital under a flesh let go 10 km off): at step 14 the walls of one building
  up to 4.8 m apart in the city (4.1 to 7.1 m up) and 5.4 m in the capital; at step 20, 20.9 m and 22.8 m. Without the fix every wall of
  a building is at its one climb (0 m apart). 7,819 to 14,695 faces a frame drawn wholly under the cover in the cover's look.
- The unit's "ruins under an overgrowth": the town's colliders the same with the cover and without, its pieces the same grown twice.
- The frames worth a look: `shots/tests/overgrowth/ruins_city_14.png`, `ruins_city_20.png`, `ruins_capital_14.png`.
