# src/space: the view from the ship

## Files
- `space_view.h` (16 KB): `SpaceRenderer`, `DetailPlate` (the near body's plate); `space_internal.h` what its files share (the
  band's shades)
- `space_view.cpp` (30 KB): `render`, the stars, the band and the kept backdrop, the flight band, `mapFor`/`mapIfReady` (a body's
  map), the season, the palette, `drawSkyBodies`
- `space_sun.cpp` (26 KB): `drawSun` (by angle), the lens flare, a black hole (`drawBlackHole`)
- `space_globe.cpp` (57 KB): `drawGlobe`, a world's banks, the screen boxes, the telescope's plate (`updatePlate`, `plateFill`),
  the lights of a dark world
- `space_bodies.cpp` (21 KB): a belt's rocks, a companion's accretion stream, a stripped giant's tail, a comet's tails
- `probe_view.h/.cpp` (60 KB): `ProbeView`, `renderProbeView`: the probe's camera inside a giant, composited over the space frame

## Rules and traps
- The backdrop is kept while the camera is still. `SpaceRenderer::BackdropKey` (space_view.h:169) holds every input of that
  pass (the camera, `Proj`, the size, `bandGen`): a new input belongs in it, and every write of `bandMap` bumps `bandGen`.
- A body's map builds on its own thread: `mapIfReady` is null until it is done, `mapFor` waits. Frame paths ask; they never wait.
- Maps and BodyGens follow the season: `setSeason` before rendering a system at a time t.
- The banks (space_view.h:112; `docs/reference/02-*` has the table): 0 stars, 1 sun, 2/3 bodies A/B, 6/7 water, 11 and 13-15
  body A, 16-19 body B, 12 the companion, 20 belt rocks, 8-10 the cabin.
- What is in the sky is where the maths puts it. The sun is drawn by its angle per pixel; a globe is culled only behind the
  camera; depth is in metres.
- Band-pinned passes: `drawStrippedTail` sums its splats per block in order; `drawSun`'s glow loop copies what it reads into
  band locals (read through the closure it ran 5x slower under one stack alignment).
- `DetailPlate` is body-fixed, with the old plate as its fallback; its blocks land in the same frames whoever builds them.
- `renderProbeView` composites RGB layers over the space frame (null means unseen); `probeSpaceShows` 0 skips the space render.

More, with the numbers: `docs/reference/lessons.md`, "Space".
