# R-429: the garden trimmed

status: done 2026-11-18 (1.5.0, before its release; one of five requests aboard, R-427 to R-431)
milestone: 1.5.0

## Request

The user, 2026-11-18: "Trimming trees, then they grow back", then: "Regarding trees/bushes in the central room - I suggest to make
them grow to a certain level on the stardrifter and let you trim them back to inital level. Just small funny interaction, I am talking
about plants at each corner." Asked how fast they should grow back (by time played, or by game time faster): "By time played".

What was there (I-04): the hall's four corner beds grew by the game's days since the expedition began, near full after two months of
game time; a game eleven hours in had them at 36% of their height against 35% planted.

## Done

- Each bed grows by the time played since it was last trimmed: a third of its height then (as planted), its full height an hour of
  play on, eased in and out, and no further. E at a bed (`FACE_BED_1` to `_4`, faced from toward the stair) trims it back; at its
  start it says there is nothing to trim yet.
- The time played counts the real seconds of every state but the title; it and each bed's trim are the save's `garden` line (a save
  before it plays from 0, its beds as planted).
- Proof and records: `docs/decisions/log/7-1.5.0.md`, R-429; `ship life`'s `garden_planted`, `garden_grown`, `garden_trimmed`; the
  unit's `ship` group.
