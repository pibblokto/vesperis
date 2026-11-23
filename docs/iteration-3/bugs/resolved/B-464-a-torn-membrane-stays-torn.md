# B-464: a torn membrane stays torn

status: resolved 2026-11-22 (1.5.0, before its release)
severity: S4
area: surface (`surface/overgrowth.cpp`: `updateGrowthSongs`; `overgrowth_ruins.cpp`: `growPieces`, `membraneKey`; `forms_draw.cpp`)

## Report

KI-479, found in J-10 (2026-11-22): "A torn membrane is heard, never seen: it is drawn whole again behind the explorer." The user listed
it with the others to fix (2026-11-22): "Now let's handle all of these".

## Cause

`updateGrowthSongs` heard the explorer cross a membrane's plane and set the tear's sound, but kept nothing: the membrane is a piece of
its cell's cover, drawn whole every frame, and pushing through it again tore it again. The cell's pieces are laid out again at a
re-anchor and when the cover is grown again, so a mark on the piece alone would be lost.

## Fix

A membrane torn is kept for the landing in `tornMembranes`, by its cell's own key (a settlement's grid cell, a work's id) and its
opening's index among the cell's openings: the same for the same opening however often the cell is laid out. Its piece is marked
(`Form::broken`), and a cell grown again marks it from the set. A torn membrane is drawn slit up its middle as high as the explorer went
through, ragged at the edges, its sheet whole over the slit (its own raster parameters: cut out under its weight, by how far across from
its middle). It tears once: crossing it again makes no sound. The set is kept across a re-anchor and emptied by a new landing.

## Verification (2026-11-22)

- The unit's "ruins under an overgrowth", extended: four doorways' membranes in the scene's town pushed through from 2.5 m, four torn,
  four marked torn, four still torn once the cell is grown again, and the first pushed through again tears nothing. Without the fix: none
  marked, none still torn, and the first torn again.
