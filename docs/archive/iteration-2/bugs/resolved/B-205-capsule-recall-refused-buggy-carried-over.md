---
id: B-205
title: K refuses to recall the capsule after driving; the buggy carries over to other landings
severity: S2
status: resolved 2026-09-27 (N4-06)
reported: 2026-09-27
---

## Where
State / screen: surface, after a drive; and every landing after a buggy was unfolded once.
Save: `vesperis_save_auto.txt` of 2026-09-26 shows it (`buggy 1 2275.7 399.9 ...` on a site whose capsule is at 0,0).

## Steps
1. Unfold the buggy, drive, get out, press `K`: "THE BUGGY IS N M AWAY - DRIVE IT BACK BEFORE RECALLING THE CAPSULE".
2. Board the capsule, land on another planet (or another site): the buggy is "deployed" there too, at the old local coordinates, kilometres away; `K` is refused again; the launch log repeats the same "DROVE 2451 M".

## Expected
`K` always brings the capsule; a new landing starts without a buggy; `B` at the capsule always gives a fresh buggy.

## Cause
`SurfaceView::init` reset the player and the capsule but never the `Buggy`, so `deployed`, the coordinates and the odometer survived every landing; `K` had a rule refusing the recall while the buggy was more than 200 m away, which the stale buggy always triggered; `deployBuggy` refused while one was out.

## Fix
`init` resets the buggy (and `inBuggy`, `chaseCam`); `reanchor` moves the buggy with the frame like the capsule; `K` always relocates the capsule (the status names a buggy left over 50 m away); `B` at the capsule always unfolds a new buggy and the old one is scrapped; the launch log counts the metres driven this landing over every buggy (`Game::drivenLanding`). Checks: `unit` ("B replaces a buggy left far away", "no buggy on a new landing"), `flow` (recall with the buggy out, `B` redeploys, a second landing has no buggy).
