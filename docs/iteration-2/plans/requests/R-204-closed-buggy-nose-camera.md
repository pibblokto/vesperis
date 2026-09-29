---
id: R-204
title: A closed, futuristic buggy seen from inside through its nose camera
status: done 2026-09-27 (N4-06)
requested: 2026-09-27
related: N4-06, B-203, B-204, B-205, R-203 (the open buggy it replaces)
---

## What
Rework the buggy into a closed futuristic vehicle (no open frame). Inside it you see only an interface with a CCTV camera effect, as if the camera on the front of the buggy were its way of seeing outside: you look with its eyes. The camera can only be turned around the front (no looking back while driving, it confuses).

## Why
The open-frame cabin shook, its wheel turned the wrong way and showed odd squares (B-203, B-204); the user wants the driving view to be the vehicle's own camera.

## Must / should / could
- must: a closed hull in the parked and chase views; from the seat nothing but the camera picture and its overlay; the camera pans a limited angle each way, never backwards.
- should: a recognisable CCTV look (grain, scanlines, lifted blacks); the readouts (speed, gear, trip, heading, capsule, steering) on the overlay.
- could: a visible camera pod on the nose, a REC light, a pan gauge.

## References
The user's message of 2026-09-27; rover cameras.

## My notes (filled by Claude)
Done as N4-06 in `surface/buggy.cpp`, `surface_view.cpp` (the camera), `game/hud.cpp` (the feed hookup and the interface). Model: a wedge hull (nose 0.84 m wide at 1.62 m, shoulders 1.72 m wide, a 1.5 m roof over the cabin, a raked tail) with a blacked-out sensor band round the cabin and across its front, wheel pods (five arc strips out to 1.06 m plus a skirt) over the four knobby wheels on their suspension arms, a camera pod on the nose with a lens dot and a red LED that blinks while the buggy is live, a lidar puck on the roof, a sensor mast with the blinking light, a headlight bar and a tail light bar, chrome shoulder and roof lines; it grows out of the folded state over two seconds. Camera: the eye sits in the pod (1.58 m ahead, 0.84 m up in the hull's frame), pans +-70 degrees and tilts -35..+25 (`SurfaceView::CAM_PAN`, `CAM_TILT_DOWN`, `CAM_TILT_UP`, applied in `updateBuggy` so the pan is kept relative to the hull as it turns), takes the chassis' pitch and roll, trembles smoothly (B-203) and dips after thumps. Picture: `SurfaceView::cameraFeed` after the mush: palette blacks lifted to 16/255, saturation x0.45 with a cold cast, then per pixel scanlines (x0.82 on odd logical rows), a corner vignette (down to 30%), +-1.5 shades of noise and a 5% brighter band rolling down every 4.5 s. Interface (`renderSurfaceHUD`): inner brackets, `CAM 01 NOSE` with a blinking `REC`, `PAN`/`TILT`, a pan gauge under the compass, a crosshair, the speed in double-size digits with the gear and the trip, a steering bar (right is right), heading, state (`AIRBORNE`/`SKID`/`BRAKE`/`LIGHTS ON`/temperature), the capsule and the local time. `V` keeps the outside chase view. Frames: `scene buggy_seat`, `scene buggy_parked`, `drive_seat`, `drive_chase`, `drive_night[_chase]`, `flow_buggy_*`, `bench_buggy`; the driving frame costs 5.0 ms at 2x (was 5.7: no cabin to draw).
