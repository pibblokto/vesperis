# Rules: rendering and the frame

The rules in force for the framebuffer, the palette, the rasteriser and the frame, a line each with the entries behind them
(`log/`, by release). A milestone that sets or changes one edits its line here; a replaced one moves to `superseded.md`
(`docs/CLAUDE.md`). The order, threads and time: `rules-rendering-frame.md`.

## The pixel and the palette
- A pixel is a 5-bit bank and an 11-bit intensity (32 a shade): writers use banks and shades, never colours [M10-03], [O0-02], [B-305]
- The mush averages intensities, never across banks (edges melt, no colour bleeds); two hues take two banks, never a blend [table], [B-403]
- Resolution is a setting (1x-4x, `setFramebufferScale`), not a build variant; the user's default is 4x with the fine 3x3 mush [table], [M10-01], [B-310]
- The HUD draws after the mush in RGB, in logical units; the 5x7 font has no lowercase, all text in capitals [table], [M10-01], [C-02]
- Scene light (the veiling glare) is drawn before the mush, never as HUD lines; only the probe's view composites RGB [B-309], [X-02]

## Banks and the light across them
- Banks are allocated per view (`setupPalette`, `02-rendering.md`'s tables), not from one pool: a new use takes a free one [R-404], [W-03]
- Each world with a disc owns its banks (A 2, 6, 11, 13-15, 31; B 3, 7, 16-19; belt rocks 20): two in a frame never share one [O0-02], [B-305], [Q-02]
- Lent banks: trait colours 25-27 only without a people (`TraitDef::moves`), floraless accents grass and forest, sunless aurorae 12 [Q-02], [P-00], [D-02]
- Ramps that meet agree at the shades met (the mush averages across banks); light and depth scale colours, never shades [B-408], [X-01], [X-02]
- A bank over the sky starts at its colour (aurora 11, 21, band 22: no hue edge); an airless sky lies all in 22, sun and glows [B-403], [G-03], [B-408]
- A pixel changes bank by luminance read from the palette, never by shade number; light added to a sky is summed so [B-408], [B-427]
- An overlay takes its bank where its light beats the pixel's (band 22, nebulae; the tail dithered 4x4); `glowDisc` only on empty pixels [G-03], [B-427]
- The sky bank is black under shade 30, dark to 44: what is drawn in it uses its bright shades (earthshine 30-42) [O0-01], [B-301], [W-04], [B-427]

## Light
- A new effect is absent to the bit where it does not apply (no front, one sun, lamps by day, `litShare` 0): added after the old result [W-03], [B-425], [D-04]
- A second sun blends into the one `env.sun`, the only sun shading reads; shadows keep at least 40% of direct light [M5-01], [M10-06]
- Only the world's own sun is floored at 0.62 (`SurfaceSite::sun`), not the other sun, globe or maps; a drifter has no sun [O6-02], [B-425], [D-01]
- The band is toned on a fixed exposure (`bandToneOf`, `BAND_EXPOSURE`), never normalised or adapted in flight [G-03], [G-07]
- `bodyShineAt` is earthshine and planetshine, scaled by the sun's factor; sunless, the band alone lights a globe, never above it [W-04], [D-01], [D-02]
- Each ground light formula, a new lit object's too, adds (1 - ambient) x `lampAt` by night, not a screen pass; `floodlights` only for the game's capsule [D-04]

## One world in every view
- The sky is where the maths puts it, from ship and ground alike; both views pass `drawSun` the orbit's normal and `discUp` [B-428], [S-04], [S-05]
- Suns draw by angle per pixel; a globe is culled only wholly behind the camera, else boxed exactly (`sphereBox`, `ringBox`) [B-308], [O0-03], [B-304], [B-312]
- One material palette, tint path and exposure rule in every view (`matColor`, `waterTint`, `exposureStop`): globe, map, ground agree [N1], [B-202], [Q-02]
- The landing map takes the globe's shadows per texel; relief shading subtracts the slope toward the star, map and plate alike [O0-04], [B-306], [B-407]
- The telescope's `DetailPlate` samples the planet function body-fixed; plate and zoom draw what the ground has, none exaggerated [W-06 reworked], [T-01]
- The telescope is the window's view at a multiplied focal length (`Game::telescopeProj`); only it sets `detailBody` [W-06], [W-06 reworked]
- One `SurfaceView` only: the drone screen draws schematics, never a second camera [C-18]

## Small things and the mush
- No visible squares: grain perspective-correct and bilinear past 2.5 px a texel, material edges noisy, smooth shading by default [table], [B-311]
- A point of light is a square `FB_SCALE` + 2 px wide in its glow's stops: a lone pixel dies in the mush [D-04]
- A sub-pixel tower is one pixel at its own shade, never widened or paled; forms fog as terrain does; a window cell under two 1x px is its mean [A-02]
- Against shimmer: far thin features are sized in screen pixels, never texels; sampling grids are anchored in the world [P-05], [P-06], [X-02], [G-05]

[table]: log/0-1.0.0.md
[M10-01]: log/0-1.0.0.md
[M10-03]: log/0-1.0.0.md
[M10-06]: log/0-1.0.0.md
[M5-01]: log/0-1.0.0.md
[N1]: log/0-1.0.0.md
[B-202]: log/0-1.0.0.md
[O0-01]: log/0-1.0.0.md
[B-301]: log/0-1.0.0.md
[O0-02]: log/0-1.0.0.md
[B-305]: log/0-1.0.0.md
[O0-03]: log/0-1.0.0.md
[B-304]: log/0-1.0.0.md
[O0-04]: log/0-1.0.0.md
[B-306]: log/0-1.0.0.md
[O6-02]: log/0-1.0.0.md
[B-308]: log/0-1.0.0.md
[B-309]: log/0-1.0.0.md
[B-310]: log/0-1.0.0.md
[B-311]: log/0-1.0.0.md
[B-312]: log/0-1.0.0.md
[B-403]: log/1-1.1.0.md
[G-03]: log/1-1.1.0.md
[S-04]: log/1-1.1.0.md
[S-05]: log/1-1.1.0.md
[C-02]: log/1-1.1.0.md
[W-06]: log/2-1.2.0.md
[W-06 reworked]: log/2-1.2.0.md
[R-404]: log/2-1.2.0.md
[W-03]: log/2-1.2.0.md
[B-407]: log/2-1.2.0.md
[B-408]: log/2-1.2.0.md
[W-04]: log/2-1.2.0.md
[X-01]: log/2-1.2.0.md
[X-02]: log/2-1.2.0.md
[G-05]: log/3-1.3.0.md
[G-07]: log/3-1.3.0.md
[A-02]: log/3-1.3.0.md
[T-01]: log/3-1.3.0.md
[C-18]: log/3-1.3.0.md
[B-425]: log/5-1.3.2.md
[B-427]: log/5-1.3.2.md
[B-428]: log/5-1.3.2.md
[P-00]: log/6-1.4.0.md
[P-05]: log/6-1.4.0.md
[P-06]: log/6-1.4.0.md
[Q-02]: log/6-1.4.0.md
[D-01]: log/6-1.4.0.md
[D-02]: log/6-1.4.0.md
[D-04]: log/6-1.4.0.md
