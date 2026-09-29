---
id: B-321
title: Still can't find animals
severity: S3
status: resolved 2026-09-28
reported: 2026-09-28
---

## Where
State / screen: surface, living worlds
Star / body shown in the HUD: any
EPOC shown in the HUD: n/a
Save attached: no

## Steps
1. Land on a living world and look for animals.

## Expected
Herds within walking distance on most landings, and a way to find them.

## Actual
"Still can't find animals" (the user), after B-316 put habitat herds on 512 m cells.

## Notes
Three causes, measured with `vesperis_test encounters 300` and the new `climate`:

* **Two thirds of the felisian land was ice sheet.** The biome temperature ran from the body's *equilibrium*
  temperature with no greenhouse (an Earth-like orbit came out at -10 C), its latitude line `28 - 0.55 lat` was five
  degrees too cold in the middle latitudes, the snow line fell 40 m per degree of latitude (every hill over 500 m was
  snow from 50 degrees on), and the continental base of `3200 land^0.8` put plateaus at 2-3 km, over that line.
  Now: living worlds keep 33 K over their equilibrium temperature (`BodyGen::tempBias`), the latitude curve is
  Earth's (`climateTempC`: `-18 + 46 cos(lat)^1.5`, 27 C at the equator, 5 at 50 degrees, -9 at 70), the snow line
  falls with the square of the latitude (5000 m at the equator, 3000 at 45, 1000 at 65, gone at 85), the base is
  `1500 land^0.7` and the mountain amplitude 1500-4000 m (2500-5500). The alpine biome begins at the treeline (half
  the snow line, 700 m at least) instead of 700 m everywhere. Over 30 worlds the surface is 44% sea, 18% ice sheet
  (the snowball worlds included), 9% alpine rock and the rest living land, where it was 41 / 26 / 15.
* **Herds were thin.** The habitat chances rose by a third (grassland and savanna 0.72, forests and wetland 0.52,
  taiga 0.4, desert and tundra 0.26, alpine 0.14): 2.2 habitat herds (10 animals) within 900 m of a random land site
  where there were 0.7 (3), and a herd within 200 m of 34% of random land sites (14%).
* **Nothing pointed at them.** The HUD's suit readout `LIFE 640 M NW` names the nearest herd or flock within 1.5 km
  (`LIFE HERE` under 25 m, `(FLYERS)` for a flock); `X` still brackets what is in view.

Also: the HUD's temperature comes from the same climate model as the biomes (with a day-night swing of ten degrees);
it used to read -10 C on a grassland plateau from a model of its own.
