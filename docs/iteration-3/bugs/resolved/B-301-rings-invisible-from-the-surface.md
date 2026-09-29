---
id: B-301
title: Rings are invisible from the surface (drawn near black on the sky bank)
severity: S3
status: resolved 2026-09-27 (O0-01)
reported: 2026-09-27 (verbal: "Are rings of the planet visible from its surface? They should be, maybe this feature is already present but I couldn't find such a planet")
---

## Where
State / screen: surface, any ringed world; also a moon of a ringed world.

## Expected
A ringed world shows its ring as a bright pale arc across the sky, by day like the daytime Moon, at night brighter, with the world's own shadow cutting it; a moon of a ringed giant shows the parent's rings across its disc.

## Actual
The arc existed since M5-04 (`shots/rings_surface_day.png` before this fix: a dark grey band on black) but was drawn at shades 20-40 on the sky bank, whose airless ramp is black up to shade 30 and dark grey to 44. The rings of a parent seen from a moon (`drawGlobe` in sky mode) had the same mapping (`63 dens^0.4 ...` blended at 66%, landing at 15-25). Ringed solid worlds were also rare (7% of solid planets) and nothing on the landing map said whether the ring stands in a site's sky.

## Analysis (Claude)
`drawSky` computed `rv = 63 pow(dens 1.3, 0.35) (0.6 + 0.4 litRing) shadow lightFactor` and, with an atmosphere, `rv (1 - 0.35 day) + v 0.5 day`; for a typical density of 0.5 this is 30-45. The bank-1 ramps put their bright part at 43-63 (the moons in the sky use `43 + 20 lv` for that reason). So the arc was there, but as the darkest grey the ramp has.

## Fix (O0-01)
* `drawSky`: the arc's value is `36 + 24 pow(dens 1.3, 0.35) lit` with `lit = (0.55 + 0.45 |axis . sun|) x face x max(lightFactor, 0.5)`, where `face` is 1 when the sun is on the observer's side of the ring plane and 0.5 for the unlit face (light comes through); the part in the world's shadow is not drawn at all; with an atmosphere `rv (1 - 0.3 day) + v 0.45 day`. `drawGlobe` in sky mode maps the parent's rings to `36 + 27 ...` the same way.
* The ring profile moved to `StarSystem::ringProfileOf` (shared by the globe, the sky arc and the ground) and `SurfaceSite::sun` computes `ringShadow`: the sun ray from the site crossing the ring plane inside the ring dims the light by `0.8 x 0.85 dens` and the sky by 45%; the HUD says `RING SHADOW`.
* The landing map's `SKY:` line says `RINGS 62 deg UP`, `RINGS EDGE-ON OVERHEAD` or `RINGS BELOW THE HORIZON` for the world's own ring, and marks a ringed body in the sky as `RINGED`.
* Generation version 5: solid planets get rings with 12% instead of 7% (`rng.chance(0.12)`, the same draw, so nothing else in a system changes; the regression's generation hashes did not move).

## Verification
`vesperis_test moonsky`: `shots/rings_surface_day.png` and `_night.png` show a broad pale band, `moonsky_rings.png` the parent's bright bands; `unit`: a ringed landable planet exists near the start, its profile is bounded with a dense part, and a site at low latitude at some time of the year stands in its ring shadow (0.30). Near the start 22% of landable bodies are moons of a ringed world (so the ring-in-the-sky sight was already common from moons; the surface arc of a ringed planet is what was missing).
