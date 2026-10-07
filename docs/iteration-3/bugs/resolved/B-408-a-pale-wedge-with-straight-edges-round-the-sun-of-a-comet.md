# B-408: a pale wedge with straight edges round the sun of a comet

status: resolved 2026-10-07
severity: S2
area: surface, the airless sky (the galactic band and the suns' glow)

## Report

The user, landed on Saieli comet II (an orange giant's comet, sector 129 -136 -47, 81.1 S 43.8 W, R 2 km, venting 100%):
"that weird rectangular light or something in the sky", three screenshots. A broad band of pale blue light crossed
the sky through the sun with straight sides, a small triangle of the same beside it, both cut off by a horizontal
line a little above the ground; where the band crossed the sun, the disc was mottled and grey, the other half smooth
and bright; a thin light line ran along the band's edges.

## Cause

Two banks met under one glow. The galactic band (G-03) lies over the sky in bank 22, written where its light beats the
sky pixel's own, and the sun's disc, corona and outer glow are drawn after it, adding their shades in the bank the
pixel already holds. On an airless world the sky bank's ramp is black up to shade 30 (the glow's tail, the halo the sky
draws round the sun and a comet's coma all lie there unseen), while the band's ramp rises from black at once: the same
glow showed as a broad pale fan wherever the band's patch lay and not at all beside it, so the patch's own edges became
the fan's edges. On a comet they were worst: the coma lifts the whole sky's shade (to 20 near the sun) without lifting
its light, so the band's patch ended along the contour where its light passed the coma's (the straight sides, the
triangle, the arc), and the mush, which averages shades across an edge, made a pale rim of every edge (a band pixel at
shade 4 beside a sky pixel at 20 averages to 12 in the band's ramp, brighter than either). The disc is never dimmer than
the glow-lit sky round it, read from the shade under it: the coma's 20 shades washed the disc white on the sky's side and
the band's few shades left it mottled on the band's. The horizontal cut was the band's own test `d.y <= 0`: on a body
of 2 km the ground falls away within a hundred metres and the sky shows under the horizontal all round.

The same edge was on every airless world where the band's patch ended inside a sun's glow (the wings of the band under
1.5 shades were left to the sky, so a large black arc cut the faint band in many a black day sky), it only rarely
crossed a sun.

## Fix

`SurfaceView::drawSky` (`surface/surface_view.cpp`):

* **An airless sky is the band's bank all over.** The band pass writes every sky pixel above the ground's horizon in
  bank 22, the band's shades added to the sky's own light carried over by its light (a table from the sky ramp's
  intensities to the band ramp's of equal luminance: the coma, the halo round the sun and a ringed world's ring arc keep
  what they looked like); the suns' glows, drawn after it as before, land on one bank whatever lies behind them, and the
  band's faint wings run on to nothing instead of ending at 1.5 shades.
* **The sun in the same bank.** On an airless sky the primary's disc, glow, eclipse corona and flare are drawn in bank 22,
  whose ramp keeps the band's star-grey under shade 20, turns to the sun's colour above it (at the grey's light) and from
  55 up takes the sky bank's own stops, so the disc keeps its colour and its edge blends into its glow (the sky bank's
  grey corona shades between them had made a dark ring). The pixels the disc covers keep the sky's shade, which the disc
  reads to stay as bright as before. A black hole's sky and a protostar's keep the sky bank for their star: the hole has
  no light and its lens shows what lies behind it, and the protostar's glow lands on its cloud's tint over the whole sky
  as S-04 drew it; for a hole the halo the airless sky draws round the star is carried over as no light.
* **Down to the horizon of the drawn ground, not to the horizontal**: on a small body to the bottom of the frame (the ground
  drawn after hides it), on a planet to the floor's horizon (`floorHeight`, the floor's height now computed once a frame
  before the sky for both, `floorH`).
* **Behind the bodies**: a sky body's night side keeps its pixel, as the stars already did (the band used to show through
  a dark moon).

Worlds with an atmosphere are drawn as before (their band only shows at night, and their sky ramp has no black foot):
the scenes `thinatmo_sunset`, `felisian_sunset`, `thinatmo_aurora` and `front_far` are bit for bit the same.

## Verified

The user's place (`spot 129 -136 -47 7 -81.1 -43.8 3767000 stand 12 272 scale=4`, and 295 degrees at t 3763800): the
sun a bright smooth disc in a round warm glow, the band faint behind it, no edge (the disc's edge reads 238, 208, 157,
132 across in luminance where it read 238, 46, 127). The Wolf-Rayet moon the user landed on the same day
(`spot 131 -136 -49 1 53.0 -113.2 3733019 stand 30 225`): the shell's ring as before, the band's arc cut gone; the scenes
`black_hole_sky` (the lens and the disc as before), `protostar_night` (the sky as before; the ring arc in the cloud's
tint), `wolf_rayet_day`, `icy_noon` (a dark moon now in front of the band), `cratered_noon` (a comet's tail in the sky
now whole), `comet_day`, `comet_night`, `molten_night`. The regress: `scene_cratered_noon` and `scene_molten_night`
(both airless) re-blessed, the other twenty unchanged. Nothing generated changes (`GEN_VERSION` 11).
