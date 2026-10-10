# R-410: a radar turned up to hear farther, through the noise

status: done 2026-10-09 (C-15 2026-10-08, C-16 2026-10-09; `docs/ideas/ITERATION-1.3.0.md`)
milestone: 1.3.0

## Request

"Also there should be a way to increase sensitivity of signals radar so you can find further signals but it should give some
radio noise, creepy mixed sounds when you are trying to get really far signal, etc"

The radar (C-07) hears the peoples' transmitters within 200 ly (`SIGNAL_REACH_LY`) and the loud pulsars within 25, at one
sensitivity; what it hears is as old as it is far. Measured on 2026-10-08 with a scratch program over `signalsNear` from
home: 3 peoples' signals within 200 ly (1 ms), 9 within 400, 59 within 800 (7 ms), 234 within 1,600 (30 ms). A longer reach
is cheap but would make the signals common, so the sensitivity works through a power per transmitter (C-15).

## Plan

* **C-15, deep listening** (after A-01, which leaves the radio ages alone on the air): a gain in the radar camera (x1 to
  x32 in six steps; x1 hears what C-07 hears under A-01's rule),
  a hashed power per transmitter that decides how far the gain carries it, the far scan on a worker, the static's floor and
  the hold to lock growing with the gain, a strong near signal saturating.
* **C-16, what the gain lets in:** the bed rising, the galaxy's own noise in the beam (the band and the core loud), the
  signals of the wide lobe mixed under the centred one with heterodynes and intermodulation, far voices worn (a narrow band,
  fades, echoes, drift), more and stranger events in the static at the top steps, false rises, the picture's ghosts; the
  levels set from rendered WAVs; and each age on the air (spark-gap Morse and a phonograph, voices and music, data, a beacon
  that is mathematics), built in the same pass.

## Done

* **C-15 (2026-10-08), the gain.** In the radar camera `+`/`-`, the wheel and `PgUp`/`PgDn` step a gain of x1, x5, x11, x18,
  x25 and x32 (`GAIN X11` by the scope). x1 hears exactly what the radar heard before.
  * Every transmitter has a power: C-07's are weak, and a sparse population of strong ones is heard to about 7,000 ly at
    x32. Beyond 200 ly a people is heard only where the light reaching the ship left after its first broadcast.
  * The steps are even, so each adds about two peoples' signals. The salts are pinned so every step adds one to three from
    home and from the test worlds; from home the farthest is 5,524 ly.
  * The price: the static's floor climbs and fills the scope, so a far signal is a small rise over it. A near signal
    overloads the receiver (the meter pinned red, the lock refused, the sound overdriven), so the gain is turned down for it.
  * The hold to lock grows to eleven seconds for a faint far signal at x32 and the cones narrow.
  * A far signal locked flies like any (thousands of light years on the readout) and is on the galaxy chart.

  The noise and the creepy mixed sounds are C-16 (KI-371). `docs/reference/12-civilisations.md` and `10-decisions.md` (C-15).
* **C-16 (2026-10-09), what the gain lets in.** Turned up, the receiver lets the void through, and each age is on the air.
  * The static rises: its hiss and a crackle grow, the rumble breathes harder, and the band of the galaxy is a rush in the
    beam, loudest toward the core, where a far signal is hardest to pull out.
  * A second signal in the wide lobe is heard under the one centred: two broadcasts at once, a pulsar under a voice, a
    whistle between two carriers that slides as the beam turns, and at the top steps a metallic ring.
  * A far signal comes worn: a narrower band, its pitch drifting, deep slow fades, dropouts and echoes of itself.
  * The static's events come oftener, and at the top steps stranger ones join: speech running backwards, counting
    whispered in no one's language, a breath at the microphone, a knock answering a knock, a choir that stops dead. A
    word of a recording already heard comes back out of the static.
  * False rises: the meter rises where nothing is; held, the hold fills and empties, `NOTHING THERE`.
  * Each age on the air: the industrial a phonograph and Morse by spark, the modern C-07's programmes, the futuristic a
    synthetic voice, data and beacons, the far future a beacon of mathematics.
  * The picture's static, the band's glow, a ghost double and tears grow with the gain.

  `docs/reference/12-civilisations.md` (what the gain lets in, with where to listen from home) and `10-decisions.md` (C-16).
