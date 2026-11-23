# B-490: a flock's and a shoal's changed young and sick

status: resolved 2026-11-23 (J-16)
severity: S4
area: surface (`surface/bestiary.cpp`: `animalFateOf`, `changeFits`; `surface/creatures.cpp`: `flockMembers`, `drawLife`, `updateLife`'s
falls, `swimmersNear`, `updateHabitat`'s washing up, `keepDead`)

## Report

KI-495, found in B-469 (2026-11-22): "A flock's and a shoal's changed young show the body's and the hide's changes alone (none of the
ways'), a crazed flyer only wobbles, and a sick flyer or swimmer never falls dead on its own." The user asked for every known issue of
1.5.0 to be handled (2026-11-23).

## Cause

- `changeFits` gave a flyer's or a swimmer's young none of the ways' changes (calm, bold, by night, circling, stumbling, climbing,
  swarming, limping, joined), since a flock's and a shoal's members have no ways of their own.
- A crazed flock member only had a wobble added to its place (a few metres either way); nothing ever brought it down, as a land animal's
  crazed falls do.
- The one sickness that kills during a landing is sprouting, and `animalFateOf` gave it to land animals alone (B-469: "none sprouts in
  the air or the water"), so no flyer or swimmer ever died on its own.

## Fix

- **Ways in the air and the water.** A flyer's or a swimmer's young can now be born circling (it goes round on its own, 3 m about its
  place) or limping (it trails 5 m behind its place, bobbing; a swimmer at half its pace); a flyer's can stumble (it drops 3 m in some
  of its four seconds' windows and catches itself) or be born to the night (after dark it flies on over its resting flock). The changes
  no flock or shoal can show stay out: calm, bold, climbing, swarming, joined, and for a swimmer stumbling and the night.
- **A crazed flyer falls.** In some of its four seconds' windows (as a land one's, 35% of them), it falls to the ground in a second,
  lies there beating its wings for a second and a half, and flies up again.
- **Sprouting in the air and the water.** A sprouting fate now reaches a flyer and a swimmer where the line meets it. A sprouting flock
  member falls dead at its time (four to sixteen steps after its fate came, by its hash, as a herd's): it drops from where it flies,
  turning over, and its remains are kept where it comes down, its stalks up (in the water it sinks). A sprouting swimmer past its time
  is gone from the water and lies washed up dead on the nearest dry ground within 60 m of where it swam, staged by the time since its
  fall, its stalks up, kept; one with no shore so near sank.
- The flock's members are placed by one function (`flockMembers`) for the frame and the update alike, from the flock's own draws in the
  old order, so a flock nobody's line reached is drawn as before, bit for bit.

## Verification (2026-11-23)

- The unit's `animals` group, new check "a flock's and a shoal's changed young and sick (B-490)". Before (the new check on the old
  rules): "0 flyers' and 0 swimmers' species with a way's change among their young's; NO SPROUTING WORLD". After: 22 flyers' and 7
  swimmers' species with a way's change over 56 seedings at step 40; the sprouting recipe's world has 2 flyers' and 2 swimmers' species
  sprouting; a crazed flock comes down to 0.0 m over the ground (the same flock whole: 30.2 m at its lowest), one of it lying there 57 s
  of a minute; of a sprouting flock of 12, all 12 fell dead at their time, kept with their stalks; 10 sprouting swimmers past their fall
  gone from the water, all 10 washed up on dry ground, kept and staged by the time since their fall. (The sprouting world is a
  terminator world whose sea has no shore within 40 km, so the fall and the washing up are watched on another seeded world's shore with
  the sprouting laid on its flock and its swimmers by hand.)
- B-469's "the flyers' and the swimmers' fates" still passes; it no longer asks that none sprouts.
- `seed fates`: the sprouting recipe's flyers and swimmers now read "40% of the rest SPROUTING" (a glider, a flyer, a ray and an eel),
  and a soarer's changed young CIRCLES where it was PALE; its 79 frames are the same bit for bit (none shows a flock near enough to
  tell).
- `regress`: 24 hashes, none changed.
