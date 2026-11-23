# B-485: the ground follows the steps

status: resolved 2026-11-23 (J-16)
severity: S4
area: surface (`surface/overgrowth_ruins.cpp`: `adoptMadeNow`, `madeGroundKey`; `surface_view.cpp`: `update`, `startResample`, `takeResample`,
`joinAhead`; `surface_view.h`: `setMadeNow`)

## Report

KI-491, found in B-465 (2026-11-22): "On the ground the ruins follow the steps (`ruinsMade`), but the ground's cover, its structures and
the made looks keep the landing's layer: the cover's edge stays where it was at the landing." Listed for J-16 on the user's word
(2026-11-23): "after that there should be a J-16 that will basically handle all K-xxx that were created".

## Cause

The ground takes the world's layer once, at the landing (`site.gen.made`). B-465 had the game hand the world now to the ruins alone
(`setMadeNow`). The terrain's samples, the structures and the made looks were made from the landing's layer and never read again, so
an explorer who stayed on the ground saw the ruins taken further while the cover's edge beside them stood still.

## Fix

- When the game hands on a world now, the ground looks at what its samples read (`madeGroundKey`: the layer's key without the animals'
  fates and the steps that the ruins and the remains count on past the spread). Where that moved, the ground takes the world now
  (`adoptMadeNow`, in `update` after the ruins): the site's layer, the cover's tiles made again, the activity vision's square metres read
  again. Once a world's spread, looks and climate are done, a step passing changes nothing on the ground but the ruins.
- Every ring round the camera is sampled again on a worker, with the corners of the structures' world cells in view (`startResample`);
  45 frames on, the main thread copies them in (`takeResample`, waiting for the worker if it is not done) and the structures are
  placed again from those corners. The frames between draw the ground as it was, so the cover's edge moves without a frame that samples
  everything, and on the same frame however fast the worker was (the frames repeat). Cells out of the rings are dropped and sampled when
  next asked for. `joinAhead` stops the worker where the site's roads, streets or works change: the cells it did not reach are dropped and
  sampled by the frame, alike.
- The herds already about keep what they were (those that come into reach after the step read the world now). A runaway's seas that
  boil off while the explorer stands there are gone from the ground too.

## Verification (2026-11-23)

- The unit's `seeding` group, new check "the ground's cover and structures follow the steps": a landing on a steppe 25 km from a flesh's
  place. At its 9th step (its cover 5 km out) none of 289 points round the landing is covered and no structure stands; its 12th handed
  on (63 km out): all 289 covered, 380 structures, the ground's layer the world now. With the ground kept on the landing's layer, as
  before: 0 covered and 0 structures.
- B-465's check now also reads the ground's layer as the world now; its climb is as before (450 m at the 11th step, 1,015 m at the 14th).
- The cost, timed with the town scene at night on a machine shared with four other runs: a frame 5.3 ms; the frame a step was handed on
  4.8 ms; the worst of the 45 after it 33.9 ms, the one that took the worker's rings in and waited for it (the 45 frames ran in 0.36 s
  here; at 60 frames a second they take 0.75 s). Sampling every ring in the step's own frame, as first written, took that frame to
  194 ms. A step that moved only the ruins left the ground's key as it was.
