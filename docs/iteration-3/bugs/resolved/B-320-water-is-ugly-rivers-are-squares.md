---
id: B-320
title: Water is ugly, rivers are squares
severity: S2
status: resolved 2026-09-28
reported: 2026-09-28
---

## Where
State / screen: surface, living worlds
Star / body shown in the HUD: ZIOGRYAND II-A 4.7 S 191.6 W (the screenshot), any
EPOC shown in the HUD: 6011:003.853
Save attached: no

## Steps
1. Walk over a grassland with a river or a lake in sight.

## Expected
A river: a ribbon of water in a valley, flat across, following the slope along; a lake: one flat sheet with a shore.
Water that reads as water.

## Actual
"Water is ugly and also rivers are squares" (the user, with a screenshot of pale squares scattered over a plain and a
blocky pale strip in the distance).

## Notes
The old rivers (M9-04) were a channel four to ten metres deep cut into the bumpy ground along a hashed network, with
the water level set *per vertex* at that vertex's own height plus a few metres, and the old lakes a sheet draped a
few metres over the basin floor: the water surface followed every bump, and a cell was drawn as a flat quad at the
highest of its corners' levels, so a river narrower than a cell came out as isolated squares and a lake as a stair of
plates.

Now (`planetmap.cpp`, `felisianGround`/`felisianSample`, GEN_VERSION 8):

* the felisian ground is one function returning the ground and the *smooth ground* `hS` (the relief spectrum's
  octaves of 2 km and longer, `ReliefSum::hCoarse`, over the base); every inland water body is cut into `hS`;
* rivers: two networks (the boundaries of 80 km and 25 km worley cells, the second a third as wide and deep), each
  river a ribbon of 5-110 m half-width (wider in the lowlands and the wet, a hashed factor per river) with a
  floodplain where the fine relief melts into `hS`, a channel 2.5-11.5 m deep and the water a metre under `hS`:
  flat across, following the valley along; the plane extends under the banks (`SurfaceSample::water` over the whole
  zone) and the ground above it wins by depth; sand bars where the bank lies within 1.2 m of the water;
* lakes: hashed features (`lakeAt`) on 7 km cells with the body's `lakeDensity` (0.35-0.8), radii 0.4-1.3 km,
  outlines that wobble with the angle, and one level each: the smooth ground at the lake's centre less two metres
  (`lakeLevelAt`, the felisian ground evaluated at the centre at 100 m detail, cached per thread), so the sheet is
  flat and the shore is where the ground crosses it; the bowl's shore plain sits two metres over the water;
* wetland pools cut into `hS` the same way; rift-valley lakes on worlds with the rift trait;
* the drawing (`drawTerrainLOD`): the water level per corner from the vertex cache (`VtxCache::wy`, morphed with the
  ground at the ring edges like the heights, `coarseWaterAt`), a corner without water taking the mean of the others,
  so the sheet is one plane cut by the ground; `SurfaceSite::waterAt` bilinear over the wet corners; the reflection
  pass mirrors the shore cells of a lake or river round that water's level, not the sea's;
* the look: a Fresnel term (the water's own colour from above, deep dark and shallow lighter; the sky's colour at
  grazing angles, so the far water mirrors the sky), the sun's glint and the wind's waves as before, and the water
  ramp darkened with the night (it used to be as blue at midnight as at noon).

Scenes `felisian_river` and `felisian_lake` (`SurfaceSample::waterKind` finds them), `vesperis_test water` counts the
inland water of the first worlds per thousand samples at 16 m (rivers 0.6-2.0, lakes 0.3-2.3 where there were 0.3-0.9
and 0.1-1.0 before the tributaries). Open: from the capsule and the air the shore of a lake on the 64 m ring is a
stair of 64 m steps (KI-321).
