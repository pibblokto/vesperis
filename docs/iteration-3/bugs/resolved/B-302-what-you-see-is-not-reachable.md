---
id: B-302
title: What you see may not be reachable; no sense of distance on the ground
severity: S2
status: resolved 2026-09-27 (O1)
reported: 2026-09-27 (verbal, with a screenshot that arrived as a file icon: "I landed in a 'mountains' region, spawned on the mountain with the shore downhill; no matter how far I went the mountain never ended, I never reached the shore. A lot of things that spawn are just decoration; whatever you see you should be able to reach")
---

## Where
State / screen: surface, a felisian world, a site of relief class MOUNTAINS with water in sight.

## Analysis (Claude)
Nothing drawn on the ground was fake, but three things made a shore in sight unreachable in practice:

1. **Draw distance and haze.** The terrain stopped at the 512 m ring, 13.3 km out; a felisian haze of 5 km faded everything beyond 8-10 km to the horizon colour. From the descent camera (1800 m up) a shore 8-12 km away is clearly visible; on the ground it is a faint blue patch or gone, and a mountain range of the planet function is 300 km wide (the tectonic chain band) with its ridges every 35-45 km, so walking downhill for a kilometre changes nothing you can see.
2. **No sense of distance.** The HUD gave no range to what you look at and no time to get there; on foot 4.2 m/s makes 10 km a 40-minute walk, in the buggy 6-8 minutes over rough ground. The waypoint (`M`) could only be set where you stand.
3. **Scale of the map.** The landing map's "sector" was a 1 x 1.5 degree cell (about 130 x 200 km) drawn 1.4 px wide; the coast next to the cursor was a hundred kilometres from the site. That part is milestone O2 (`requests/R-301`).

## Fix (O1)
* A fourth terrain ring: 2048 m cells to 26 cells (53 km), coarse-to-fine prefetch, its own vertex cache; the haze opened to let it show (felisian 12 km, ocean 9, quartz 5, thin air 14, airless worlds 45, molten and volcanic 30; venusian stays 0.7 km).
* The ground's curvature (`d^2 / 2R`, before only on the terrain mesh) moved into `SurfaceView::toView`, so rocks, trees, the capsule, the buggy, creatures and the water all sit on the curved ground; the floor beyond the last ring is a fan of curved rings out to ten times the far radius instead of a flat 40 km plate hanging at eye level.
* A rangefinder (`SurfaceView::rangeToGround`): the line of sight is marched over the caches (bilinear lod0 near, the coarser rings further) with the curvature; the HUD shows `8.4 KM  33 MIN WALK` (or `6 MIN DRIVE`) under the compass, `WATER` when the crosshair rests on water.
* `M` puts the waypoint on the spot under the crosshair, however far ("WAYPOINT SET 8.4 KM AWAY"); the capsule and waypoint readouts print kilometres.

## Verification
`unit`: 45 degrees down from eye height ranges 2.3 m, the far ring reaches 53 km; `flow`: the range readout after landing (670 m, 3 min walk in the frame); `bench check`: surface 2x 3.9 ms (3.4 before the far ring), descent first frame 16 ms; regress frame hashes re-blessed (the curvature now moves every object by the same drop as the ground; the haze changed every distant pixel).
