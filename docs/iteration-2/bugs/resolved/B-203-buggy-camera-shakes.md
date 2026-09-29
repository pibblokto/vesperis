---
id: B-203
title: The buggy shakes violently while driving
severity: S3
status: resolved 2026-09-27 (N4-06)
reported: 2026-09-27
---

## Where
State / screen: surface, in the buggy (seat view), any world.

## Steps
1. Unfold the buggy, get in, drive at speed over rough ground.

## Expected
A steady picture with a little tremor.

## Actual
The whole view jumps every frame, worse with speed and roughness.

## Cause
`SurfaceView::render` added `Rng::sym(vibration x 0.04)` to the eye height and `sym(vibration x 0.012)` to the pitch with a fresh seed every frame: white noise of up to +-24 cm and +-1.4 degrees at 60 Hz.

## Fix
The camera (now the nose camera, R-204) trembles as a smooth sum of sines (7.3 and 11.9 Hz on the height, 9.1 Hz on the pitch) with an amplitude of at most 1.5 cm and 0.4 degrees, and dips after a thump through the new `Buggy::jolt` that decays at 5/s. No random offsets remain.
