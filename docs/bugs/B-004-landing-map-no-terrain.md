---
id: B-004
title: landing map shows only day and night areas, no terrain, on some planets
severity: S2
status: fixed (2026-09-26)
reported: 2026-09-26
reporter: user
---

## Where
State / screen: LANDING_MAP
Star / body: seen on several planets (a molten world in the scripted flow, `shots/flow_landing_map2.png`, shows it)
EPOC: any
Save attached: no

## Steps
1. Orbit a molten, venusian, rocky, icy or quartz world and press `C`.

## Expected
A map with visible relief and surface features so a landing sector can be chosen.

## Actual
A flat colour split into a lit area and a dark area; no terrain visible. Felisian and thin-atmosphere worlds look fine because their maps mix several materials.

## Notes
Frequency: always on single-material worlds.

## My notes (Claude)
**Cause.** `Game::renderLandingMap` colours each texel as `materialColour * (0.75 + 0.35 * albedo) * dayNight` (`src/game/game.cpp:573`). On worlds where the whole map is one material (basalt, rock, ice, quartz, dust), the only variation is the albedo term, compressed into a 0.75-1.10 range, so the map is a flat tint with the terminator on top. Lava on molten worlds is a small fraction of texels and lava is drawn by material colour only. Venusian worlds are additionally covered by clouds in the orbital view, and their map draws the uniform basalt underneath.

**Fix.**
1. Contrast-stretch the albedo per body (min/max over the map, then 0.35-1.0) instead of the fixed compression.
2. Add hillshading from the height map (finite differences toward a fixed light direction) so ridges, craters and domes read even where albedo is flat.
3. Draw lava (glow) as bright orange regardless of lighting; draw ice fractures and maria with their own tints.
4. For atmosphere worlds, overlay the cloud pattern and let `C` toggle it (the original's cloud filter), so venusian worlds show clouds by default and the ground on demand.
5. Optional: a 2x zoom window around the cursor showing the exact terrain sampled by the site probe (which already reports material and elevation).

**Size.** M. **Check.** `flow_landing_map*.png` regenerated for a molten, an icy and a venusian world; relief visible in all three.

**Fixed 2026-09-26.** `Game::buildLandingMapBase` computes the base colours once per body: material tint x albedo stretched between its 2nd and 98th percentiles x a north-west hillshade whose strength is normalised by the map's RMS gradient (so every body gets the same relief), lava drawn emissive (unaffected by night), clouds overlaid at 55% on felisian/thin-atmosphere/quartz worlds and as an opaque deck on venusian worlds; `C` toggles the clouds (the original's cloud filter), `Esc` leaves the map. The day/night term is applied per frame from a cached table of texel directions. New test mode `vesperis_test landmaps` writes `shots/landmap_<TYPE>.png` for every landable type; all eight show relief. The zoom window (item 5) was not done.
