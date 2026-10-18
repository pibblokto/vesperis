# B-427: a white blob behind a Wolf-Rayet star's giants

status: resolved 2026-10-18 (1.3.2, the fifth of the patch's items)
severity: S3
area: the envelope the wind strips from a giant (`space/space_view.cpp`, `drawStrippedTail`)

## Report

The user, with three screenshots from the gas giant of PARSIS +18071 +1 -19950 (`S12+S04 MULTIPLE`, its body 1; EPOC 6011:003.810),
one in standby 0.24 MKM off and two parked in orbit 0.17 MKM off (`OVER 1.9° S 112.1° E`), a white glow beside the giant on the side
away from its sun: "So as you see this gas giant has this strange white lights behind it. I noticed i with several gas giants as well
and I have no idea why is it this way and with them specifically - it's just a blob of white light behind them. Please fix".

## Cause

* **It was the wind's tail, drawn as a blob.** S12 is a Wolf-Rayet star, and S-05 gave every gas giant and substellar object of one its
  envelope streaming away from the star: forty glow discs of the giant's bank along a curve from the limb to four radii out, widening
  and thinning, before the globe. Those are the "several gas giants": the giants of Wolf-Rayet stars (40 of the 4,559 stars near home;
  R-419's Stargazer and Giants seek them out). Three faults made the tail a white light:
  * **Its light heaped up.** Each disc added up to 14 shades, and their centres crowd at the root (the curve runs out as the square of
    its parameter: its first third covers a tenth of its length), so a dozen overlapped beside the limb and their sum reached the top of
    the ramp, which goes from the giant's colour at 47 to white at 63. At 4x, 20,000 to 42,000 pixels round the giant stood at the top
    shade wherever the tail showed.
  * **It took the sky's colour.** A glow disc takes its own bank only on an empty pixel and adds to the pixel's own elsewhere. Behind the
    giant lay the band's glow (bank 0, blue-grey), so where the tail was not white it was the band's blue.
  * **It hid behind the globe.** Seen from the night side the tail streams toward the camera, in front of the globe, but the globe was
    drawn over it; only the edge round the limb showed: a crescent, or a ring over the top of the disc like an eclipse's corona.
* **In a moon's sky** the discs were drawn in the sky's bank, which is black below shade 30 on an airless world (every world of a
  Wolf-Rayet star is): the tail showed only where the discs heaped past 30, as a patch with a hard edge.

## Fix

* **Each stretch of the tube carries its own light.** Splats about half their half-width apart along the same curve (about ten), each
  weighted by the length of axis it stands for: seen side-on, the tail's column is the profile's, 24 shades at the root falling as
  `(1 - f)^1.3`. A view down its length adds up to a soft ceiling of 36 (`36 (1 - e^(-sum / 36))`), against the lit face's 40 to 50 and
  the white at 63.
* **After the globe, behind what is nearer.** Each splat is hidden where the frame holds something nearer than the splat, softly over
  the splat's own depth. From the sun's side the globe hides the root; from the night side the tail veils the dark face.
* **Its own colour.** The tail's light goes to the giant's bank where it outshines the pixel's own and stays in the pixel's bank where it
  is the fainter. Where neither outshines the other, the two banks are dithered (an ordered 4 x 4 pattern between the shares 0.3 and 0.7):
  a pixel has one bank, and the plain switch drew a hard outline where the tail crossed the band.
* **Filaments along the flow.** Each splat's light varies across it by two octaves of noise, streaming outward with time, so the tail is
  streaks of gas rather than a smooth tube. The streaks fade as the tail turns end-on, where "across" has no direction.
* **In a moon's sky by luminance.** The sky's bank is read from the palette: the tail adds at most the luminance of its shade 40 (the
  giant's lit side is drawn from 43 up) to the pixel's, and the sum is written as the bank's shade of that luminance. The band's pass
  then carries it into the band's bank by luminance, like the sky's own pixels. From a moon, the tail is a pale plume rising off the
  limb.
* **The cost.** About ten splats in place of forty discs, sampled once per 2 x 2 block at 3x and 4x (the finest filament spans tens of
  pixels there), in bands of rows on the workers, each block summed over the splats in order. The picture is the same with one thread.
* No generation change.

## Verification (2026-10-18)

* `vesperis_test tail` (new, `08-testing-and-tools.md`): the reported giant from its parking, carried round it by Shift and the arrows
  as a player is (a frame every 45 degrees), then from 12 and 40 radii, from a moon at quadrature (the giant 50 degrees up, the sun under
  the horizon), through the cabin's window and at 1x. The pixels at the top shade round the giant, every 45 degrees from the parking:
  1,931 / 3 / 4,167 / 27,756 / 42,548 / 20,968 / 42,147 / 29,519 before, 145 / 3 / 89 / 11 / 0 / 110 / 0 / 6 after (what is left is
  the giant's own lit limb). The tail now covers 17,000 to 182,000 pixels a frame in the giant's bank, at a mean shade of 8 to 16. From
  the night side it lies over the dark face. From 12 and 40 radii it streams like a comet's tail, in the giant's tan. From the moon it is
  a grey plume rising off the limb, which was not drawn at all before the luminance step.
* The 13 frames are the same from two runs and from one thread (`pngdiff`).
* `vesperis_test unit`, a new check (B-427): the reported giant from 2.5 radii on the sun's side, side-on and on the night side, no
  stars drawn. Off the globe: no pixel at shade 48 and up in its bank or the band's, and 42,830 / 83,667 / 121,265 pixels in its bank
  at 6 to 48. A second tail drawn over the finished frame lights 243 / 4,103 / 132,157 of the globe's 500,000 pixels: hidden from the
  sun's side, over the dark face from the night side. On the old drawing the check fails on every count: 145,815 pixels at 48 and up,
  19,096 / 16,967 / 3,887 in its bank, a second tail lighting 114,438 / 63,353 / 145,638 of the globe.
* `unit` 339 checks, 0 failures (one new); `regress` 24 of 24 unchanged (none of its scenes has a Wolf-Rayet giant); `flow` passes.
