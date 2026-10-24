---
name: look
description: Inspect Vesperis's harness pictures and sounds without spending the context on them - which PNGs to open, crops, montages and stats with tools/png.py, dark frames measured rather than eyeballed, WAV statistics with tools/wav.py, pngdiff.
---

# Look at the harness's pictures and sounds

**Where they are:** `shots/tests/` (every harness output, about 2 GB: never copy it whole); a snapshot's in
`build/prove/<name>/items/<item>/shots/`, a compare's in `build/prove/cmp/<name>/<item>/shots/`; `shots/` itself holds the
player's screenshots.

## Pictures

- Open PNGs with Read, at most about eight in a response. A 4x frame (1280x800) costs as much as many 1x ones: crop it first.
- `tools/png.py montage <out.png> <cols> <a.png> ...`: many frames as one picture to Read (each cut to the first one's size).
- `tools/png.py crop <in> <out> x y w h [scale]`: a detail, scaled up by whole pixels.
- `tools/png.py stats <a.png> [x y w h]`: each channel's mean, min and max, the luminance's quartiles and the share darker than
  16. Judge a dark frame (night, a rogue world, the dark ocean) by these numbers, never by eye: the Read tool's view hides the
  low steps of the ramps.
- `tools/png.py diff <a> <b> <out.png>`: the differing pixels in red over a darkened copy of a, with the count, the largest step
  and the rows. For whole directories: `./vesperis_test pngdiff <dirA> <dirB>` (the counts by eighths of the height).
- `./vesperis_test bothways <scene> [scale]` draws a scene with the workers and with one thread and compares them.

## Sounds

- `tools/wav.py stats <a.wav> ...`: the length, the peak, the rms, the samples at the ceiling, the loudest second.
- `tools/wav.py events <a.wav> [threshold]`: the sound's events (start, length, peak, rms, the zero-crossing pitch at the start,
  the middle and the end).
- `tools/wav.py pitch <a.wav> [from to]`: the autocorrelation pitch of the loudest 50 ms.
- The harness prints the peak and rms of what it renders; there is no numpy here (python's `wave` module only).

## Naming what you see

Nothing has a name until the explorer gives one: the HUD says UNKNOWN. To say where a screenshot was taken, try every body and
read the HUD (sector, body, latitude and longitude) before naming it.
