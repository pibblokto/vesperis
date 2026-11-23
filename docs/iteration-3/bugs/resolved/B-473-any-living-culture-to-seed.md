# B-473: any living culture to seed

status: resolved 2026-11-22 (1.5.0, before its release)
severity: S3
area: game (game/seed_trip.cpp: the seed page's culture; game/drone_state.h)

## Report

The user, 2026-11-22: "Select of culture on drone doesn't work." Then, on the plan: "I don't like that you have a window for culture to
be seeded - once it's grown you should be able to seed it".

## Cause

C on the seed page offered only a flask whose culture showed FULL at that moment. A culture held FULL for 5 to 15 minutes, then rested
or died and left the list, and a new strain took 10 to 20 minutes to become FULL. With one full flask, C chose the same one again and
seemed to do nothing; with none, the page said NO FLASK SHOWS FULL.

## Fix

- C goes through every culture alive in a flask, each of a flask's two in turn, whatever its stage: growing, full, starving, or asleep
  as spores. The page opens on the culture last chosen while it lives, else the first that shows FULL, else the first alive. Only an empty flask or a dead culture is passed over. The page's CULTURE line says the stage ("A ROD, FLASK 2, GROWING"),
  `NO OTHER CULTURE IN THE FLASKS` when there is only one, and `NO CULTURE IN THE FLASKS` when there is none.
- With R-449 a culture once full stays full, and it is full in 2 to 4.5 minutes at its best.
- The verdict is the line's and the place's, as before: a culture sent young takes or dies as a full one would.

## Verification (2026-11-22)

- The unit's `seeding` group, new check "the seed page offers every culture alive in a flask and says its state": flask 1 the dye line
  just put in, flask 2 empty, flask 3 the line full. The page opens on flask 3 (FULL), C goes to flask 1 (GROWING), C again back to
  flask 3, and from flask 1 Enter sends the young culture; with every flask empty the page says NO CULTURE IN THE FLASKS and C and Enter
  are refused. By the old code flask 1 was never offered.
- A first cut opened the page on the first culture alive: the flow's seed stage then sent flask 1's encysting amoeba, not its full dye
  line (CULTURE DIED, TOO COLD). The page now opens on the culture last chosen while it lives, else the first that shows FULL.
