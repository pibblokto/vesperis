---
id: B-310
title: The picture is too mushy: a higher resolution with the original's feel
severity: S2
status: resolved 2026-09-27
reported: 2026-09-27
---

## Where
State / screen: everywhere (the default render scale)
Star / body shown in the HUD: n/a
EPOC shown in the HUD: n/a
Save attached: no

## Steps
1. Play at the defaults: a 1280 x 800 window, render scale 2 (640 x 400), the classic 3 x 3 mush.

## Expected
Detail: the relief of O6-02, the trees, the ground textures.

## Actual
Everything melts: the 3 x 3 box twice at 640 x 400 spreads over 2.5 logical pixels, and each framebuffer pixel is a
2 x 2 block of the window. "Super mushy" (the user).

## Notes
The user asked for a higher resolution that keeps the original feeling. `vesperis_test compare` renders eight scenes
with six configurations and `sheet` shows them as the 1280 x 800 window would (`shots/tests/scale_<scene>.png`):
A 2x with the 3 x 3 box (the old default), B/C 3x with 3 x 3 and 4 x 4, D/E/F 4x with 3 x 3, 4 x 4 and 5 x 5. The
decision is D: **render scale 4 (1280 x 800, one framebuffer pixel per window pixel) with the new "fine" mush box of
3 x 3** (`mushMode` 3 = `max(2, scale - 1)`): the melt is still there at the pixel level (two passes span five
pixels, 1.25 logical pixels) but leaves, trunks, rocks and ridges resolve. The other configurations remain in the
settings; a settings file from before takes the new look once (header version 2) and keeps everything else.

Cost, and what was done about it (an Apple M-series, `bench`): a 4x surface frame was 8-12 ms and a dense tropical
forest 28 ms. The terrain rings and the flora are now drawn in parallel bands of rows (`setRasterBand`: every fill of
a thread is clipped to its rows; the vertex caches and the material tiles are filled first, in parallel by rows of
cells, so the band threads only read; the trees are listed once per frame with the rows each can touch; a canopy blob
wholly outside a band is skipped before its points are placed; the results are identical to the serial loop's), the
per-pixel passes run on a persistent worker pool instead of threads created per region (a frame dispatches a dozen
regions), and the canopy spray takes its sample points from one hashed table and fills them directly (the per-point
hashes, roots and trigonometry were most of a forest frame). The `bench` prints the surface render's sections at each
scale and has 4x rows for the forest and the buggy with budgets of 24 ms.
