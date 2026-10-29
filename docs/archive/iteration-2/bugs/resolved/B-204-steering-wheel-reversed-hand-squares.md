---
id: B-204
title: The steering wheel turns the wrong way and shows squares on it
severity: S3
status: resolved 2026-09-27 (N4-06: the cabin is gone)
reported: 2026-09-27
---

## Where
State / screen: surface, in the buggy (seat view).

## Steps
1. Get in the buggy, press `D`.

## Expected
The wheel turns clockwise; hands on it.

## Actual
The wheel turns counter-clockwise; two flat grey squares float on the rim.

## Cause
The wheel's in-plane basis was `ex = cross(axis, up)`, which points left, so `rot = -steer x 2.6` rotated the rim the wrong way; the "gloved hands" were two untextured quads.

## Fix
R-204 replaced the drawn cabin with the nose camera's picture: there is no wheel to draw. The steering shows as a bar on the camera interface whose marker moves right when steering right (verified by the direction of `Buggy::steer`, positive for `D`).
