# B-457: a colour a line

status: resolved 2026-11-22 (1.5.0, before its release)
severity: S4
area: galaxy and surface (galaxy/planetmap.cpp: `madeGround`, `madeGrowth`, `madeColourAt`, `madeMatColour`; life/seeding.cpp:
`madeWorldOf`; surface/surface_palette.cpp and the drawing of the ground, the trees, the structures, the ruins' cover;
game/landing_map_build.cpp)

## Report

Two known issues of one cause, the user asking for them fixed (2026-11-22, with the other known issues of the J series):

- KI-460, found in J-05: "One colour a class: two lines with looks in one class (two dyes, a milk and a rust, crusts and sulphur) draw
  the later's colour wherever either lives."
- KI-473, found in J-09: "One overgrowth colour a world: where two growing lines meet, both covers take the latest line's colour and bank
  (each keeps its own type, tile and structures)."

## Cause

A world's layer kept one look a class (its waters' tint, its land's, snow's and plants' looks), the latest line's that made one, and one
overgrowth colour, the latest growing line's. The made pass knew which line lay at a point but kept only its share, so every view drew
the class's one colour.

## Fix

- Each line keeps its own colours in its spread: its waters' (a dye's, a milk's, a rust's, else the world's water under its glow), its
  land's, snow's and plants' looks, and its own glow (`MadeSpread::waterTint`, `look`, `glow`); its overgrowth already had its own.
- The made pass records the line whose look or overgrowth lies at a point (`SurfaceSample::madeLine`), and the planet map, the
  telescope's plates and the ground's vertices keep it (`mline`, a byte the vertex had spare).
- `madeColourAt` and `madeMatColour` take the line: the maps' ramps are made a line each (`MAP_LINE`), the landing map's zoom, the sector
  map and the drone's map read the texel's.
- On the ground each line whose colour differs takes a bank of its own from the free ones (B-456), the latest line's first; two of one
  colour share one, and one past the free banks takes the latest line's. The ground's cover, its gloss and its veins' light, the waters'
  tint and its glow, the trees and the undergrowth of a plants' look, the wrapped trees, the covered rocks, the structures (each from
  the line whose cover lies at its cell's middle: its type, its sizes, its colour) and the animals taken in read the line where they stand;
  a settlement's cover takes the line at its middle.
- On the globe each disc's banks are a line's too (B-458).
- Where several lines lie, the line whose look shows a colour wins over one whose look does not (a dye's or a waters' look over a line
  that only lights its waters, a tinted land look over one that only pits the rock); among those, the one of the greater share, the later
  on a tie. The battery's `seed traits` found a red dye's waters drawn in a glowing line's plain water before this was added.
- A line's glow lights its plants only where its plants' look is a glowing host's (`MadeSpread::plantsGlow`): a glowing line's blight or
  frost burn stays dark at night, on the globe and on the ground, and the trees' canopies are lifted by their own line's glow.

A world of one line draws as before.

## Verification (2026-11-22)

- The unit's `seeding` group, new check "a colour a line": `seed record`'s overgrowing line, a red one and a blue one let go on opposite
  sides of its world. Its waters and its overgrowth are each drawn in their own line's colour where that line lies (every point of each,
  none in the other's). By the old code both lines' ground took the latest line's colour.
- New check "each line's banks on the ground and the globe": landed by the red line, the two lines' overgrowth and waters each in a bank
  of its own; on the globe, as either disc, each line's stain and waters in banks of their own.
- New check "a line's glow in its own plants alone": a line whose waters glow, with a blight on its plants, beside a glowing host. The
  blight's bank is dark at night and the host's lit. With a line's whole glow read for its plants, as first written, the blight was lit.
