# B-502: the electron's slide kept while still

status: resolved 2026-11-23 (J-16)
severity: S4
area: life and game (life/morph_mesh.cpp: `drawElectron`; game/microscope_draw.cpp: `microscopeElectron`)

## Report

KI-437, found in Z-04 (2026-11-17): "The microscope's electron view over large cells costs 25-47 ms a frame at 4x (`lab time`: the
meshes' vertices at their cap, built every frame); unbudgeted. A cap by the view's share, or meshes kept while the stage is still."

## Cause

Timed by its parts at 4x, the electron's frame drew its filter (the grey stub and its pores) pixel by pixel, each pixel hashing the
nine pore cells round it and the four corners of its grain: thirteen hashes a pixel, a million pixels, about a third of the frame's
work. Then it built every cell's mesh and filled it. It did all of this every frame, though under the beam the slide's clock is
stopped (a fixed, dried sample) and nothing moves until the explorer turns, tilts, zooms or moves the stage. The bench's `electron 4x`
row read 40 to 45 ms a frame.

## Fix

- The stub's pore cells and grain corners are hashed once a frame, into two tables over the view's span; each pixel reads its nine
  cells and four corners from them, with the same sums as before, bit for bit.
- The microscope keeps the electron's slide while the stage is still (`microscopeElectron`), as the catalogue already keeps its form
  under the electron: the slide's pixels and depths are kept with what drew them (the mount staged, counted by `microscopeView`; the
  view's place, scale, turn and tilt; the focus; the things done to the slide; the frame's size). While none of them has moved the
  frame lays the kept slide again; a turn, a tilt, a zoom or a move of the stage draws it afresh.

No cap: a cell's mesh is as fine as before, so no frame changed.

## Verification (2026-11-23)

- Bit for bit: `lab microscope`'s 20 frames at 1x and at 4x (154 electron slides among them) and `life_look`'s 60 frames (the stub's
  change alone) are the same as the old code's, and so are the battery's `lab` item's 32 frames and its printout at 1x (times aside).
  At 4x, on a busy machine, four differ, none by these changes: two differ between two runs of the old code too, and two by a world's map
  not yet made behind the incubator's screen (the old code's frame has its globe, the new one its dim disc).
- `lab time scale=4` (it now also gives the slide's drawing in the threads' time together, which other jobs move less), three runs of
  each, one after the other, on a machine busy with four other builds and runs (so the wall times are high): ELECTRON x200, the whole
  frame 62-76 ms by the old code, 1.5-1.9 ms now (the kept slide); the slide drawn afresh 66-90 ms, 48-67 now (the threads' time 263-267
  ms by the old stub, 183-186 now). ELECTRON x1000: the whole frame 49-51 ms, 1.6-2.2 now; drawn afresh 50-55, 38-43 now (199-212 ms
  of the threads' time, 129-139 now).
- The bench's `electron 4x` row times a still stage, so it now reads the kept slide.
- Left: a moving stage under the electron (a turn or a tilt with the mouse, a zoom easing in) still builds and fills every cell's mesh
  each frame: a new known issue.
