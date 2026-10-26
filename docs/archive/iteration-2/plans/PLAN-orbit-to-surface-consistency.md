# Plan: what you see from orbit is what you land on (N1)

## Where it stands

Shape is consistent: the globe, the landing map and the terrain sample the same planet function (`docs/reference/05-planet-function.md`), so the mountain you picked is there. Colour is not: three systems, tuned separately (analysis in `bugs/B-202`):

* the globe: `typeColor` per type with a random hue, one land bank and one ocean bank per body, brightness from the map's albedo;
* the landing map: a fixed per-material table (`matCol`) times a percentile-stretched albedo;
* the surface: `SurfaceLook` per type (`ground`, `secondary`, `tertiary`, snow) on three or four banks, with rock, sand and dust sharing the ground bank.

Brightness curves differ too: the globe uses `63 albedo^0.6 shade 1.1`, the ground `matRamp` with dark/mid/lit/fog stops and hemisphere light, the map a stretch between the 2nd and 98th albedo percentiles.

## Design

**One palette per body.** `BodyGen::matColor[MAT_COUNT]` computed in `BodyGen::make`:

```
base hue      = Body::color (typeColor)                          // the world's identity from orbit
rock          = lerp(grey-brown, base, 0.5)  x roughness tint
sand/dust     = lerp(tan, base, 0.35), dust warmer
grass/forest  = vegColor family palette (greens, blue-greens, purples, reds), season-shifted
snow/ice      = near white, ice bluer, both tinted 10% by the sky
water         = ocean blue tinted by skyTint, shallow water lighter (albedo)
lava          = fixed hot ramp; metal, sulphur, graphite, quartz: fixed tints x base
gas           = base with band contrast (gas giants, substellar)
```

The star's light colour is applied at render time (all views do this already), never baked.

**The globe (N1-02).** Banks per body A: 2 rock, 6 water, 8 vegetation, 9 sand, 10 snow/ice; body B: 3 rock, 7 water, and the shared 8-10 (the two bodies' tints averaged, the difference is small); lava, gas and substellar keep their ramps. `drawGlobe` picks the bank from `map.materialAt`. Cost: one table lookup per pixel, no map change.

**The surface (N1-03).** `materialLook` returns a bank per material family: 0 rock, 9 sand/dust, 3 vegetation (trees and grass share it with the albedo split), 2 water, 8 snow/ice, 10 metal/sulphur/graphite/quartz; `setupPalette` builds each ramp from `matColor` with the existing hemisphere light, sunset tint, overcast flattening and fog end. `lookFor` keeps sky, fog, night and cloud parameters only.

**The landing map (N1-04).** Colour = `matColor[material] x albedo` with the surface's noon shade curve, so the map is the ground seen from above; the percentile stretch stays but is applied to the same curve. A strip under the map shows the cursor's ground colour, material, biome and relief class.

**Exposure (N1-05).** One rule: the palette stop of a fully lit surface at noon is stop 47 ("lit") in every view; the globe's `63 albedo^0.6` becomes `stop(lit) x albedo^0.6` on the material bank.

## Tests

`vesperis_test consistency`: for each landable type, land at the test site, render the ground at noon looking down 30 degrees, the globe from 3.5 radii with the site at the sub-solar point (crop 24x24 px around it), and the landing map cell; convert all three through `toRGB`, average, and print the RGB distance (0-441) between map/globe, globe/ground, map/ground; write `shots/consistency_<TYPE>.png` with the three crops side by side. Threshold: under 60 for every pair after N1 (the diagnosis sheet of N0-02 records the numbers before).

## Risks

The felisian globe today reads as "the green one" from far; per-material banks make it a green-tan-white one. That is the goal, but the mush at 8 px discs will mix them; the far-body point colour stays the base hue.
