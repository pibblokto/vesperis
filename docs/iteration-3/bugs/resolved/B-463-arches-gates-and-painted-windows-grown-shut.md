# B-463: arches, gates and painted windows grown shut

status: resolved 2026-11-22 (1.5.0, before its release)
severity: S4
area: surface and peoples (`peoples/forms.cpp`: `archOpenings`; `buildings_medieval.cpp`: `genGate`; `surface/overgrowth_ruins.cpp`;
`forms_draw.cpp`: `membraneMesh`; `core/raster.cpp`: `RasterParams::patternShut`)

## Report

KI-478, found in J-10 (2026-11-22): "Only the walls' and rings' openings grow shut (doorways, C-01's windows): a gate's, an arcade's or an
arch's stay open, and painted windows just go under the cover." The user listed it with the others to fix (2026-11-22): "Now let's handle
all of these".

## Cause

Membranes grow across `RuinCell::openings`, which the buildings' code records only for a wall's doorway, C-01's window and a ring's
doorway (`Gen::opening`). An arch piece (`FK_ARCH`: a gatehouse's passage, a hall's or a colonnade's arcade, a porch, a free arch, the
works' arches) leaves its bays open in its mesh but records none, and C-01's gate is two piers and a lintel with no opening at all.
Painted windows are a pattern drawn on a face pixel by pixel; under the cover the raster simply left them out (`patternAbove`), so they
vanished instead of growing shut.

## Fix

`archOpenings` reads an arch piece's bays left standing, with `archMesh`'s own geometry: each bay's middle, half width, springing and the
head's shape and rise (a broken arch's fallen bays left out; none for one lying down or standing at heights over the reference level, an
aqueduct's). `overgrowCell` adds them to the cell's openings, and a membrane across a bay is shaped to its head (round, pointed or flat:
`FormOpening::arch`, `spring`, `rise`, drawn by `membraneMesh` in eight columns). C-01's gate records the way between its piers, under
the lintel where it stands; a palisade's gate the gap between its posts; a gatehouse whose arch fell the passage between its towers.

Painted windows grow shut: `RasterParams::patternShut` draws a window under the cover as its shape in the cover's bank at a membrane's
tone (6 shades under), nothing lit, its surround, shutters and courses gone under it, on a face wholly under the cover (its own raster
parameters, one a people's windows) and on the cover's part of a face its edge crosses.

## Verification (2026-11-22)

- The unit's `seeding` group, new check "arches and gates grown shut": an arcade of four bays has four standing, none once broken (its
  bays fallen); a gate in each of the six ages' ways has a way to grow shut, six of six (two piers, a gatehouse's arch, the passage of
  one whose arch fell, a palisade's posts); an arcade put across a building of the scene's town under a flesh fourteen steps on gets
  four membranes, each shaped to its head. Without the fix: no bay and no gate grows shut (the openings were the walls' and rings').
- `overgrowth ruins`: 2,716 to 5,847 faces a frame with their windows grown shut (none before).
- A world nobody seeded: openings are read only under a cover; the raster's new branch is never taken.
