# B-436: a small moon vanished as the view turned

status: resolved 2026-11-18 (reported 2026-11-18, before the 1.5.0 release)
severity: S3
area: a far body's point (`space/space_globe.cpp`, `SpaceRenderer::drawGlobe`: the point or the globe)

## Report

The user, on the ground at 50.4 N 92.6 E (EPOC 6011:003.638), facing west (257) under a pink day sky with a crescent moon: "when I
was on the surface there was a smaller object maybe far away moon or planet - if I was moving camera slightly down it was
disappearing". Two screenshots: the small round body under the crescent in the first; in the second, the view turned up a few
degrees (everything lower on the screen), it is gone.

The place, by the HUD and the save's vials: the user's home system (195, 0, 46), Heilya III-a (THIN ATMOSPHERE), the third world's
first moon. The crescent is Heilya III; the small body is Heilya III-c, another of its moons.

## Cause

`drawGlobe` draws a body as a point (a scale+1 square, M5-09) when its radius on the screen is under 1.2 x the scale (4.8 px at
4x), else as a globe. The radius it tested was the projected one, `f tan(angR) / cos` of its angle off the view's middle: a body
grows on the screen away from the middle.

With the user's field of 80 degrees, Heilya III-c is 4.70 px at the middle, 0.98 of the globe's least size. So it was:
- a globe 13 degrees or more off the middle (the pitch -3 and below, at EPOC 003.638);
- a point nearer it.

The two disagree by day. The globe draws its lit face (shade 47-50 over a sky of 16). The point is dimmed by the day sky
(`0.35 + 0.65 skyDark`) and drawn only where it beats the sky, so it was hidden. Turning the view up brought the body nearer the
middle, and it vanished.

## Fix

- **The choice:** a body is a point or a globe by its size at the view's middle (`f tan(angR)`), wherever it stands on the screen.
- **The point by day:** dimmed the less the nearer it comes to a globe's size (the day factor eased to 1 between 0.75 and 1 of
  it), so a near globe's lit face shows by day as the globe did.

The projected radius still sizes the HUD's marks and the point's own brightness term.

The rule (`rules-rendering.md`, "Small things and the mush"): a body is a point or a globe by its size at the view's middle.

## Verification (2026-11-18)

**A probe** of the user's place (fov 80, 4x, the game's projection for the bodies), the pitch from -12 to +8:
- before: Heilya III-c a globe down to -3 (shade 47-50 over the sky's 16), hidden from -2 up;
- after: a point at every pitch, shade 44 over the sky's 16.

**A unit check** in the `basics` group: "a moon near a globe's size shows wherever the view turns (B-436)", the same place at six
pitches from -12 to +8.
- With the old choice (a scratch build of this one change undone) it fails: 3 of 6 views, hidden at 0, +4 and +8.
- With the fix it passes: 6 of 6, the least 27.8 shades over its sky.

**The battery** against z07: each of these is z07's with this fix undone.
- `scenes`: five pixels of two biome scenes' skies, by a step of one.
- `sweeps`: the `space` sweep's `globe_FELISIAN` and its telescope frames of Wyariothmai I, a history. In its sun frame of
  Wyariothmai's star at t 1234, Wyariothmai I stood off the middle just under a globe's size. It was drawn as a tiny globe there, so
  its first generator was made then, at season -0.04. The space view keeps a world's season step by the body's index, not its
  system (KI-442, found here), and kept that generator through t 5000 and 7000. So z07's globe and the telescope's plates drew that
  season's snow and clouds; Wyariothmai I's own is +0.06 and +0.11. Now the generator is first made at t 5000, within its own step.

- `flow`: its orbit frame, 9 s after the approach, did not wait for its world's map. A world is now asked for its map once it is a
  globe at the view's middle, later than as a tiny globe off it, and the frame raced it: the world drawn flat in one run of two. The
  flow now waits for that map first (`testWaitMapOf`, the harness's rule), and the frame is z07's in three runs of three.

The rest of the battery is in the log entry.

Nothing stays open: the body is now a point at every pitch, round after the mush, where it had been a small disc when looking down.
