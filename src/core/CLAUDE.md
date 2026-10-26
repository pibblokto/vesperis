# src/core: pixels, maths and threads

The base everything draws on: 16-bit pixels in a `Framebuffer`, hashes of integer coordinates, a worker pool.

## Files
- `framebuffer.h/.cpp`: `Pix` (bank in the top 5 bits, intensity in 11), `Framebuffer`: the mush, `glowDisc`, `toRGB`, `setRamp`
- `raster.h/.cpp`: `Proj`, `RVert`, `RasterParams`, `rasterTriangle`, `rasterLine3`, `setRasterBand`
- `font.h/.cpp`: the 5x7 HUD font on an `RGBCanvas`: `drawText*`, `textWidth`, `fillRectRGB`, `blendRectRGB`
- `noise.h/.cpp`: `gnoise2/3`, `fbm2/3`, `ridged3`, `worley3`, `GrainTexture`
- `rng.h`: `mix64`, `hash2i`/`hash3i`, `unitFromHash` (`hash01`), `fnv1a`, `Rng`, `xorshift32`, `xorshiftRowStarts`
- `sphere.h`: a world's unit vectors: `arcLength`, `offsetUnit`, `offsetFlat`, `slerpUnit`, `towardUpTo`/`towardThrough`, `bearingOf`
- `clock.h`: `nowMs` (timings only); `text.h`: `lowerOf`
- `parallel.h/.cpp`: the worker pool (`pool::dispatch`, `pool::run`, `pool::inside`), `parallelFor`, `parallelBands`
- `png.h/.cpp`: `writePNG`, `writePNGAsync` (its own thread), `pngFlush`, `readPNG`
- `input.h/.cpp`: `Input`, `Key`, `PadState`; `KeyMap` and `PadMap` (bindings, load and save)
- `types.h`: `Vec3`, `Mat3`, `cameraBasis`, `RGB`, `PI`/`DEG`, `clampd`, `smoothstep`, `ifloor`, `wrapAngle`
- `fs.h`: `makeDir`, `fileExists` (portable for the Windows build)

## Rules and traps
- Everything procedural is a pure function of integer coordinates and seeds (rng.h:1): the same input gives identical output.
  A copied `Rng` forks its stream: helpers take `Rng&`; the two copies there are, are meant (`rules-galaxy.md`).
- Pixels are the same at any thread count. A pooled pass is row-pure (a row's result independent of the thread and the chunk)
  or band-pinned (`parallelBands`) (parallel.h:6). Only the main thread dispatches (`pool::contended` stays 0); a dispatch
  inside a job runs alone on its thread (`pool::inside`), so `glowDisc` and `blendRectRGB` called from a job run serially. Whoever sets `setRasterBand` (thread_local) resets it afterwards.
  After touching parallel.cpp: `vesperis_test pool`, and the unit in a `-fsanitize=thread` build.
- The look: 32 banks (material ramps) of 2048 intensities, 64 classic shades of 32 each. The mush blends intensities, never
  across banks. A point of light survives the mush only as a size-2 point (3 x 3 at 1x) at 48+ shades; on the chrome bank 7
  (a mid grey by shade 20) a dark object needs shades under 14. `setFramebufferScale` re-makes the buffers.
- The font: 6 px a character, so 52 characters a line at 320 px; lowercase is drawn as capitals; a glyph is 7 px of a 9 px line
  (an underline goes at y+7). Count a footer's characters before trusting it.
- `writePNGAsync` copies the pixels: `pngFlush` before a picture is read back, listed, or needed at exit.
- Dark frames: the image viewer stretches them. Judge by the RGB numbers (decode the PNG with python's zlib; there is no PIL) or
  print bank and shade from the harness.

More, with the numbers: `docs/reference/lessons.md`, "Pixels and the look".
