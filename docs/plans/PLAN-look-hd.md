# Plan: a richer picture, same vibe (milestone M10)

## The brief

The 320x200, four-bank, box-smoothed image is the right direction: soft, melted edges, creamy glow, flat-shaded ground with grain, a palette per scene. Keep all of that, and make the picture richer in three ways: **more definition** (resolution can go higher than 2x), **better generated textures** on the ground and objects, and **better lighting** (shadows, sky light, glow, atmosphere). Never photorealistic; when in doubt, keep the character and drop the feature.

## Vibe guardrails

What stays, whatever else changes:

* Everything renders through palette banks and intensity ramps; the mush filter averages intensities and never bleeds hue across banks. Higher precision and more banks are allowed; true-colour shading is not.
* Soft, melted edges. The kernel grows with resolution so the melt stays visible on screen.
* Grain on every surface; flat-ish shading per terrain cell (a quantised gradient at most, never smooth Phong).
* One-hue ramps per material (dark -> colour -> fog). Textures modulate intensity, not hue.
* Simple silhouettes: rocks stay pyramids, trees stay sprays, creatures stay boxes on legs. Detail comes from grain and light, not from polygon counts.
* HUD in the small font, uppercase, drawn crisp on top.
* No bloom by default, no lens dirt, no depth of field, no post-processing beyond the mush and an optional CRT.

## What was prototyped (resolution)

A temporary 640x400 build (`-DFB_SCALE=2`), a variable mush box size and a `compare`/`sheet` harness produced the labelled side-by-side sheets `shots/compare_<scene>.png` (A current 320x200 2x2; B 640x400 2x2; C 640x400 4x4; D as C + bloom) and `shots/compare2_<scene>.png` (adds E 640x400 3x3). Scenes: felisian sunset with trees and rain, cratered noon, thin-atmosphere sunset, molten night, felisian globe from orbit. **That prototype code was removed on 2026-09-26** (decision: no parallel code paths in the tree before a milestone is implemented); the sheets remain as the record. M10-01 brings resolution back as a runtime setting with one code path, and M10-02 re-creates the comparison harness on top of it.

| Variant | Verdict |
| --- | --- |
| A 320x200, 2x2 | The reference. Soft, edges melt, grain coarse, coastlines from orbit are four-pixel blobs. |
| B 640x400, 2x2 | Twice the definition, half the mush in proportion. Crisp rocks, trunks, rain, coastlines; the melt is mostly gone. Reads as modern pixel art rather than Noctis. |
| E 640x400, 3x3 | Detail resolved, edges still melt, sky and glow creamy. Matches "a little higher definition, still mushy". |
| C 640x400, 4x4 | Same softness as A with finer detail underneath; closest to the original in feel. |
| D C + bloom | Sun and lava bleed a little further; a pleasant option, not a direction. |

Cost at 640x400: 3.8 ms in space, 4.9-6.7 ms on the surface (0.9 and 1.7 ms at 320x200), the per-pixel sky being the main term.

Rule that fell out of it: **kernel = scale + 1** keeps the melt proportional (2x2 at 1x, 3x3 at 2x, 4x4 at 3x, 5x5 at 4x), two passes each.

## Resolution: how high

Support integer scales 1x to 4x at runtime:

| Scale | Frame | Kernel | Expected feel | Cost estimate (surface) |
| --- | --- | --- | --- | --- |
| 1x | 320x200 | 2x2 | classic | 1.7 ms |
| 2x | 640x400 | 3x3 | recommended default: detail plus melt | 5-7 ms |
| 3x | 960x600 | 4x4 | painterly; object silhouettes fully resolved, mush becomes a soft focus | 11-15 ms |
| 4x | 1280x800 | 5x5 | native at the default window; risk of losing the pixel feel entirely | 20-28 ms before optimisation |

3x and 4x sheets must be produced before choosing (`FB_SCALE=3` and `4` builds with the same `compare` scenes). 3x and 4x need the cost work in M10-08 (threaded rows, half-resolution sky, coarser cloud grid) to stay well under 16 ms. The default window scaling remains integer nearest where possible, with a sharp-bilinear fallback.

## Palette capacity: 16 banks

Richer textures and lighting need more hues per scene than four ramps allow (sand, rock, grass, forest, snow, water, lava, sky, clouds, vegetation, objects, capsule, creatures, glow). Proposal: **16-bit pixels, 4-bit bank (16 ramps), 12-bit intensity**. The mush keeps its semantics (average intensity, keep bank); intensity precision removes the 64-step banding in gradients; `toRGB` interpolates between palette stops. Each scene assigns its 16 ramps (surface: ground A/B, sand, rock, vegetation A/B, water, ice/snow, lava, sky, clouds, objects, creatures, capsule, glow, HUD-free spare; space: stars white/blue/red, sun, four bodies, rings, streaks, spare). This is the single enabler for most of the texture and lighting items below.

## Textures (generated, world-mapped, per material)

Today: one 64x64 grain (amplitude 9) at 3 texels per metre for everything, a fine animated grain for water. Plan: a **three-scale texture stack per material**, baked per planet seed into small tiles at first landing and mapped in world space (the rasteriser already interpolates perspective-correct coordinates):

* **Macro** (10-50 m): albedo variation from low-frequency noise, dark/light patches, wetness darkening near water, snow dusting by altitude and latitude, dust deposits in crater floors.
* **Meso** (0.5-3 m): material-specific structure. Sand: ripples (sine along the wind with noise) and pebble specks. Dust/regolith: pebble fields (worley) and small craterlets. Rock: cracks (worley edges) and layered strata on slopes. Basalt: cooled plates. Lava: crust plates with bright veins (exists) plus slow animation. Ice: crystalline facets and dark fracture lines. Snow: soft mottling and sparkle points that flicker with the sun angle. Grass: tufts (the original's "asterisms") and bare patches. Forest floor: leaf litter mottling. Quartz: translucent facets with glints. Water: two animated grains.
* **Micro** (5-20 cm): the current grain, per material amplitude, faded with distance so far ground goes smooth (a poor man's mip level).

Tiles are 128x128 signed offsets at two scales (meso and micro), selected by distance; macro is evaluated per vertex. Objects get textures too: rock facets with lichen speckle on habitable worlds, bark strokes on trunks, capsule panel lines. Decals as part of the texture stack: crater ray streaks, dune ripple fields, footprint trails (a small ring buffer of dark ellipses behind the explorer), tyre tracks when the buggy exists.

## Lighting

Today: per-vertex Lambert with a constant ambient, fog toward the ramp top, flat per quad. Plan, in the order that changes the picture most:

1. **Terrain cast shadows**: per lod0/lod1 vertex, march 6-10 samples toward the sun over the height cache and shadow the vertex if the terrain rises above the sun ray; soft edges by comparing against two sun heights. Cached per vertex and recomputed only when the sun moves by half a degree or the cache entry changes. Craters, hills and ridges get real shadow sides at low sun; airless worlds finally read at noon (KI-002).
2. **Sky light**: ambient from the sky ramp colour weighted by the normal's upward component (hemisphere lighting), plus the sun's colour on lit faces. Overcast days go flat and grey by themselves; sunsets tint the ground.
3. **Secondary lights**: lava glow lighting nearby rock (a coarse glow map from lava cells, added as emissive intensity within 30 m), planetshine and moonlight at night from the brightest body above the horizon, the capsule beacon pool of light, buggy headlights later.
4. **Speculars and glints**: quantised highlights (three steps) for water, ice, quartz and lava crust, sun glitter on water (exists), sparkle points on snow and quartz that depend on the view angle.
5. **Object shadows**: blob shadows under rocks, trees, creatures, the capsule and the explorer's feet (dark ellipses on the ground, scaled by sun altitude), which is what sells "things standing on the ground" at this resolution.
6. **Shading model**: keep flat per cell as the base, add a "quantised gradient" mode (per-vertex shade interpolated, then snapped to 8-12 steps per ramp) and compare on sheets; the grain hides the steps and the picture stops looking tiled at 2x and above.
7. **Atmospheric scattering, lite**: fog colour depends on the view direction (brighter and warmer toward the sun, bluer away), distance haze strength by pressure and humidity, sunset reddening of lit faces, twilight wedge opposite the sun. Airless worlds keep hard black skies and pin-sharp far terrain.

## Sky and atmosphere

* Two cloud layers (high thin, low cumulus) with lit tops and darker bases, drift with the wind, cast faint moving shadows on the ground (macro texture term).
* Sun with limb darkening and a corona whose size follows the star class; halos (22 degrees) in cold clear atmospheres; sun pillars at sunset on snow worlds.
* Stars with brightness classes, gentle twinkle through atmospheres, the galactic band from the density function, coloured nebula patches, the Stardrifter itself passing overhead as a slow moving point.
* Night: planetshine colour, zodiacal-light-like glow near the sun's azimuth before dawn, meteors.
* Sky rendered at half resolution and upsampled at 3x and 4x (it is smooth by nature) to pay for the rest.

## Items

| Id | Item | Size | Depends | Acceptance |
| --- | --- | --- | --- | --- |
| M10-01 | **Runtime resolution** 1x-4x (`FBW`/`FBH` runtime, LUTs, grids, texture), selectable in the settings without restart | M | M0-02 | switching in game; `bench` prints all scales. **done 2026-09-26** |
| M10-02 | **Kernel per scale** (scale + 1, two passes) and a softness setting (classic / soft / sharp); 3x and 4x comparison sheets produced and a default chosen | S | M10-01 | sheets for 3x and 4x in `shots/`; decision in `10-decisions.md`. **done 2026-09-26**: `shots/scale_*.png`, default 2x |
| M10-03 | **16-bit pixels**: 4-bit bank, 12-bit intensity, mush at full precision, interpolated palette; all writers moved to fixed-point shade | L | - | no gradient steps in sunset skies; scene bank tables in `02-rendering.md`. **done 2026-09-26** (also: coloured stars always, surface stars in their own bank) |
| M10-04 | **HUD and font at scale**: glyph scale per resolution or an 8x12 font for 2x and up; layout in scale units | M | M10-01 | legible HUD in flow frames at every scale. **done 2026-09-26** (logical units + canvas scale; an 8x12 font remains an option) |
| M10-05 | **Texture stack**: per-material macro/meso/micro generators baked to tiles per planet seed, distance selection, wetness and snow dusting, object textures | L | M10-03 | `surface` sheets per material show ripples, pebbles, cracks, tufts; no shimmering when walking. **done 2026-09-26** (`surface/textures.*`: 128x128 periodic meso tiles for sand, dust, rock, basalt, ice, snow, grass, forest, quartz; micro grain kept; both fade with distance; wet shores; rock facets textured; footprints on soft ground; snow dusting was already in the planet function) |
| M10-06 | **Terrain cast shadows** with soft edges, cached against the sun position | M | - | `surface 1` noon and sunset frames show shadowed crater interiors; cost under 1 ms per frame amortised. **done 2026-09-26** (`castShadowLit`: 9-sample march per lod0/lod1 vertex within the cached ring, soft edge, floor at 40% so low sun stays readable; recomputed per frame, about 0.3 ms) |
| M10-07 | **Sky light and sun colour** (hemisphere ambient, coloured sun on lit faces, overcast flattening) | S | M10-03 | overcast versus clear frames differ in contrast without touching the fog. **done 2026-09-26** (hemisphere ambient by the normal's up component, the ramp's dark stops take the sky colour, lit stops warm at sunset, overcast raises ambient and cuts the sun by 60%) |
| M10-08 | **Cost work for 3x/4x**: sky at half resolution, cloud grid coarser, threaded row bands for sky, mush and `toRGB`, star field in one pass | M | M10-01 | 4x surface under 12 ms on the M4. **done 2026-09-26** (`core/parallel.h` row bands for the sky, the globe tracer, the mush and `toRGB`; half-resolution sky at 3x/4x; measured 4x: space 5.4 ms, surface 6.5 ms) |
| M10-09 | **Secondary lights**: lava glow map, planetshine and moonlight, capsule beacon pool | M | M10-06 | `surface 0` night frame lit by lava; night frame under a bright parent planet. **done 2026-09-26** (lava glow from the brightest lava cell within two cells; `env.moonLight/moonDir/moonColor` from the brightest body above the horizon: phase x albedo x (angular radius / 5 deg)^2; beacon pool within 14 m at night) |
| M10-10 | **Speculars, glints, sparkle** for water, ice, snow, quartz, lava crust | S | M10-05 | frames with the sun low over ice and water. **done 2026-09-26** (three-step Blinn highlight on ice, quartz and snow; sparkle texels in the snow and quartz tiles; water glitter as before) |
| M10-11 | **Object blob shadows** (rocks, trees, creatures, capsule, explorer) | S | M10-06 | objects visibly anchored to the ground at noon. **done 2026-09-26** (`BLEND_DARKEN` ellipses stretched away from the sun, off in overcast and venusian skies) |
| M10-12 | **Quantised gradient shading** as a selectable mode next to flat, chosen on sheets | S | M10-03 | sheets with flat vs quantised at 2x. **done 2026-09-26** (settings item TERRAIN SHADING: flat cells / quantised 12 steps via `RasterParams::quantize`; flat stays the default: the original's polymap look) |
| M10-13 | **Scattering lite**: view-dependent fog colour, haze by pressure and humidity, sunset tint, twilight wedge | M | M10-03 | sunset frames warm toward the sun and cool opposite. **done 2026-09-26** (haze distance shortens 35% looking toward the sun and lengthens 25% away, thicker when cloudy; sunset tint through the ramp; a darker twilight wedge opposite a low sun in the sky) |
| M10-14 | **Sky upgrades**: two cloud layers with lit tops and ground shadows, sun limb and corona by class, halos and pillars, twinkle, galactic band, nebula patches, meteors | L | M10-08 | sky-only sheet per planet type at four times of day. **mostly done 2026-09-26**: low cumulus with lit tops / shaded bases plus a high thin veil, cloud shadows on the ground, 22-degree halo in cold clear air, star twinkle through atmospheres, galactic band (star density integrated along rays, `buildBandMap`), meteors at night. Not done: sun pillars, nebula patches, sun limb per class beyond what `drawSun` already had |
| M10-15 | **Grain, lines, points, strokes in scale units** (line thickness, point size and star dots multiplied by the scale; grain frequency per scale) | S | M10-01 | same on-screen grain size at every scale. **done 2026-09-26** |
| M10-16 | **Upscale filter and scanlines** at fractional window scales | S | M10-01 | no moire when resizing. **done 2026-09-26** (sharp bilinear: integer nearest upscale into a render texture, then bilinear; scanlines per logical row) |
| M10-17 | **Optional bloom and CRT** in the platform layer, off by default | S | M6-01 | screenshots unaffected unless asked. **done 2026-09-26** (one GLSL post shader: barrel, scanline mask, vignette; 9-tap bloom; settings CRT SHADER / BLOOM; screenshots are taken before it) |
| M10-18 | **Decision record and reference updates**; `compare`/`sheet` (re-created by M10-02) kept as the standing look test with before/after sheets for every item | S | - | docs updated per item. **done 2026-09-26** (`02-rendering.md`, `06-surface.md`, `10-decisions.md`; sheets `shots/scale_*.png`) |

Order inside the milestone: M10-03 (pixels and banks) first because everything else builds on it, then M10-01/02/04 (resolution and HUD), M10-05 (textures), M10-06/07 (shadows and sky light), M10-11 (blob shadows), M10-12 (shading mode), then the rest. Foliage spray (M1-01) and airless contrast (M1-02) are absorbed by M10-05 and M10-06.

## Prototype protocol

Every visual item ships with a before/after sheet made by the `compare` scenes (extended with a snow world, an icy moon and a quartz world), reviewed against the guardrails above. If an item makes the picture more "realistic" but less Noctis, it is toned down or dropped, and the decision is written in `10-decisions.md`.

## Risks

* M10-03 touches every writer of the framebuffer; done in one change under the frame-hash test (M7-04).
* Textures and shadows are the items most likely to drift toward realism; the guardrails and the sheets are the brake.
* 4x is expensive and may lose the pixel feel; it stays optional unless the 3x/4x sheets say otherwise.

## How to look at the prototype sheets

```
./vesperis_test compare        # renders the fixed scenes at 1x..4x with the kernel of each scale (shots/scale/)
./vesperis_test sheet          # composes shots/scale_<scene>.png (1x shown x2, 2x native, 3x/4x centre crops)
./vesperis_test flow scale=2   # any Game-based mode at a given scale
```

Decision (2026-09-26, sheets `shots/scale_*.png`): **2x with the 3x3 box is the default**; 1x stays the classic option; 3x/4x are available and go painterly (and cost 2-4x more; space at 4x needs M10-08).
