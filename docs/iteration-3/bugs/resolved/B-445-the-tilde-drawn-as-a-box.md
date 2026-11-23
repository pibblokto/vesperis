# B-445: the tilde drawn as a box

status: resolved 2026-11-20 (1.5.0, before its release)
severity: S4
area: the font (`core/font.cpp`: `glyphFor`)

## Report

KI-451, found in Y-04 (2026-11-20): the landing map and the sector map draw a lake landmark's mark, `~`, as a box, because the font
has glyphs for characters 32 to 95 only. Y-04 had used `^` for the gene editor's switch mark to avoid it. The user asked for it to be
fixed (2026-11-20: "Fix KI-451 and 452").

## Cause

`glyphFor` maps lower case to capitals, draws 32 to 95, the codes 1 to 7, `|` and `` ` ``, and returns a box for anything else. Code
126 (`~`) had no glyph.

## Fix

A 5x7 tilde glyph (`TILDE`), returned for `~`. The gene editor's switch mark is `~` again (on the map and in the EDITS line), the
same sign the guide uses to save a switch (`16~DARK`).

## Verification (2026-11-20)

- `lab genes`: `lab_genes_switch.png`'s first frame shows the `~` beside the switched gene and in "EDITS: ~ 13".
- No frame of the battery shows a lake landmark. The landing and sector maps draw its mark with `drawText`, the same path as the
  editor's mark, so the glyph above is theirs too. The battery against `y04` changed no map frame but for wording.
