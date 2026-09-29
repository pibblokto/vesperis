---
id: B-311
title: The ground is plates in ugly squares; textures need variety and smoothness
severity: S2
status: resolved 2026-09-27
reported: 2026-09-27
---

## Where
State / screen: surface
Star / body shown in the HUD: any
EPOC shown in the HUD: n/a
Save attached: no

## Steps
1. Land anywhere and look at the ground near you and at the middle distance where two materials meet.

## Expected
Ground that reads as sand, rock, grass: soft texture, irregular boundaries, variety from world to world.

## Actual
"Dozens of cases when different plates with supposedly different textures are stuck in ugly squares" (the user):
squares of texture near the feet, a checkerboard where materials meet, one flat tone per cell.

## Notes
Three causes, three fixes (`core/raster.*`, `surface/surface_view.cpp`, `surface/textures.*`):

* **Texel squares.** The micro grain (3 texels a metre times the scale) and the meso tiles (4 texels a metre) were
  read nearest-texel, and within a few metres a texel spans 20-45 pixels at 2x: a mosaic of squares the mush cannot
  melt. A texel that spans more than about two and a half pixels is now read bilinearly (`GrainTexture::atLerp`,
  integer arithmetic), nearest beyond, where texels are sub-pixel anyway.
* **Cell plates.** Since the 4 m near ring (O6-02) every cell had its own flat lambert shade from the fine relief:
  a patchwork. The default terrain shading is now the smooth gradient (per-vertex shade interpolated across the
  cell); the flat cells and the 12-step gradient remain in the settings (`shading` 0 / 1 / 2).
* **Square material boundaries.** Each cell took the majority bank of its four corners and one tile, so sand, rock
  and grass met in staircases of cells. A cell whose corners differ in family now carries two banks: the rasteriser
  interpolates a per-corner weight (perspective-correct, like the grain coordinates), wobbles it with a world-mapped
  edge noise of four texels a cell, and sends each pixel to the bank, tile and flat shade of the material on its
  side of the half-way contour. The boundary is an irregular line through the cell at every ring; the far rings
  carry the tiles too, stretched to two cells.

Variety: every material has two or three looks (`mesoVariant`, hashed per planet and material: rock cracked /
boulders / layered, sand ripples / playa / pebbly, dust pebbles / regolith / rubble, grass tufts / meadow / scrub,
snow mottled / sastrugi / crusted, ice faceted / rippled / shattered, basalt plates / ropy / blocky, metal hammered /
dendritic, sulphur crusty / lobes, graphite flaky / sooty, forest litter / roots, quartz faceted / drusy) with a
strength of 0.8-1.3 per planet, and a broad tone tile (`buildMacroTile`, periodic gradient noise, a texel every two
metres, patches of 23-85 m, +-4 shades, fading with distance) is added to every vertex's shade so one material is
not one flat tone from cell to cell.
