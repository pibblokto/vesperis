# B-503: spores held still on the plates

status: resolved 2026-11-23 (J-16)
severity: S4
area: harness (harness/modes_life_look.cpp: `panelSpecimen`, `slidePanel`, `life plates`)

## Report

KI-438, found in Z-04 (2026-11-17): "Z-02's plates draw a dormant species' spores moving as its living cell does (`life plates`: a spore
with its form's run and tumble); the microscope holds them still (`Specimen::still`). Fixing the plates moves `life_look`'s frames."

## Cause

The plates' `slidePanel` made every specimen as a living cell's. A dormant species drawn as its spores, a fossil, and their copies
round them moved with the species' own motion (a run and tumble, a glide), so a spore was drawn part way through a swim it never
makes. The microscope holds a kind that does not live on the slide still (`Specimen::still`: its jiggle alone, its beat stopped).

## Fix

`panelSpecimen` makes a panel's specimen: a spore, a cyst or a fossil drawn as it lies (act LIVE) is held still, as the microscope holds
it. That holds the forms' thin sections too (`life forms`: each plan's variants as fossils in the rock). The acts' strips (a spore
waking, and the rest) are drawn as before. `life plates` counts the spores, cysts and fossils its plates draw and the ones held still
(its place at the panel's time within a Brownian jiggle of where it lies); one that moved fails the mode.

## Verification (2026-11-23)

- `life plates 4`: by the old code "48 spores, cysts and fossils drawn, 37 held still where they lie - SOME MOVED" (exit 1); now 48 of
  48 (exit 0).
- The battery's `life_look` item moves, as the KI said: 13 of its 60 frames. `life_plate_3.png` and `life_plates_dormant.png` (their
  dormant species' spores), and eleven `life_forms_<plan>.png` (each plan's variants drawn as fossils in the thin section, which moved
  with the living cell's motion too). The other 47 are the same as the old code's.
