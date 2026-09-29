---
id: B-313
title: Hills change shape in front of you ("hills morphing"), most of all when getting out of the buggy
severity: S2
status: resolved 2026-09-28
reported: 2026-09-28
---

## Where
State / screen: surface, on foot and in the buggy
Star / body shown in the HUD: any world with relief
EPOC shown in the HUD: n/a
Save attached: no

## Steps
1. Drive across hilly ground, stop and get out; or walk toward a ridge and watch it.

## Expected
The ground keeps its shape; detail arrives without the hills moving.

## Actual
"Hills morphing, especially when getting out of the buggy: they just changed in front of your eyes" (the user).

## Notes
Two mechanisms, both gone:

* **The rings popped cell by cell.** Each terrain ring samples the planet function at its own detail (`lodFade`), so
  a coarser ring is a smoother terrain. At the pinned mountain site the unmorphed vertices at a ring's edge differed
  from the coarser ring's surface by 3 m (the 4 m ring at 120 m), 17 m (lod0 at 640 m), 84 m (lod1 at 2.3 km) and
  1015 m (lod2 at 13 km). As an edge swept over the ground with the camera, every cell crossing it jumped to the other
  ring's height: a hill at 640 m rose by 30 pixels at 4x, a range at 13 km by 20. Geomorphing now (`vertexOf`): within
  a ring's morph band a vertex slides onto the coarser ring's drawn surface (the height of its triangle at that spot,
  with the same diagonal split, and its Gouraud shade when the coarser vertices were computed this frame) by the
  vertex's continuous distance from the camera, so the two surfaces coincide at the edge and the finer relief rises as
  the edge approaches instead of popping. The band reaches from the finer ring's cover plus two cells to the edge (the
  near ring keeps a 40 m core): a six-cell band twisted the cells into a sawtooth skyline, the gap being hundreds of
  metres at the far edge. The rings are warmed coarse to fine (a ring's morph reads the coarser ring's vertex cache,
  which is warmed three cells into its skip disc for that), a coarser ring skips the cells the finer one covers up to a
  cell of overlap (it used to overlap by two or three: 4 km of both rings drawn at the far edge) and is pushed back
  0.1% in depth (`RasterParams::zbias`) so the finer ring wins where the two coincide. `SurfaceSite::groundHeight`
  follows the near ring's morph (`nearMorph`), so rocks, trees, the feet and the wheels sit on the ground that is
  drawn. `unit` proves the rings meet: the gap at every edge is 0.0000 m (`testRingSeams`).
* **The near ring faded in as the buggy stopped.** Above 12 m/s the 4 m ring melted into the 16 m ground (O6-02; the
  physics blended the same way so the buggy would not hop over 4 m bumps at 180 km/h). Braking to get out, the 4 m
  relief rose over half a second in front of you. The ring is on at every speed now, and the buggy's hull rides on a
  suspension instead (`updateBuggy`: its reference height is the ground averaged over 16 m along the way, five samples
  4 m apart, kept within the wheels' travel of the contact mean), so bumps of 4-24 m are soaked up by the wheels while
  the hull follows the ground's larger shape; on a straight slope the average is the ground itself, so the B-307
  descent rule sees what it did. `descent` (0% airborne) and `drive` (2.9 km, 172 km/h) hold.

Measuring it: `stability <scene> <metres per frame> [scale]` walks a camera 1.7 m over the ground and reports the far
band's change with a heat map; in pixels the pops are hard to tell from parallax, which is why the acceptance is the
geometric seam check. The scene `felisian_mountains` is the pinned review site of O6.
