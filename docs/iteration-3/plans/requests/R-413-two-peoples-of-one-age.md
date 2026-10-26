# R-413: the two peoples of a world of one age

status: done 2026-10-11 (released in 1.3.0)
milestone: 1.3.0 (after C-18; with C-18's case a folder each people, the same message's other answer)

## Request

Asking about a world of two peoples: "can stages of people differ? It would be nice if they were to differ not more than by 1 stage
(because it won't make sense to have futuristic society next to prehistoric). To be honest even having medieval next to industrial
doesn't make sense, it would be cool if both societes were of the same age."

Each people's age was A-01's own draw from its generator: of 600 worlds of two, 22% had both peoples of one age, 36% a stage apart and 42%
two or more (a prehistoric people beside a far-future one twelve times); the user's two worlds of two happened to agree (BUBAS, two
industrial peoples; XOHORT, two medieval). Offered the second people taking the first's age, the user: "yes, I would like planets with
two civilizations for both civilizations to be of same age".

## What was done

* **One age a world** (`peopleAgeOf`, `galaxy/peoples.cpp`): a world's second people reached its first's age. Its own draw is still
  taken (`ageOfPeople(pg, age)`: A-01's draws at fixed positions of the stream) and its result set aside, so the lengths of its eras,
  its end's years over the age's span and its radio's year stay its own draws within the shared age. Every world of one people and
  every first people stand as they were; of the 600 worlds of two, 466 (78%) have a new second people (its towns, works, records,
  instruments, history, voice and music: everything the age gives), the rest are as they were.
* **A people's generator read as its people** (`BodyGen::worldSeed`): the charts (the stars a people marked, its figures' names) and the
  records' categories past the fifty read a people's age from its own generator alone (`peopleAgeOf(pg, 0)`), which a second people's
  generator no longer says. The generator keeps its world's seed (`BodyGen::make`; C-13's `peopleGen` and the copies with a people's
  seed change `seed` alone), and `peopleAgeOf` reads a second people's generator as that people. Found by the records' check: Eleinewai
  I's second people, prehistoric now, drew a star chart's figures as the medieval people it had been ("sword" in a prehistoric record).
* **The causes by the one age**: two peoples that ended together draw their shared cause by their age (both ages' weights summed before:
  the same pick where they agreed); a people conquered by the other needs no later age of it.
* **The radar**: a world of two transmits where one of its peoples had radio, 33.5% of the worlds of two (47.8% before; all civilised
  worlds 31.3%, 34.2% before), and the gain's step from x11 to x18 added no people at home. The cells (the near transmitters, home's
  three peoples) and the strong transmitters (C-15's far signals) are kept; the weak transmitters' powers are pinned again
  (`SALT_POWER` 0xC15A0404, of 8,000 tried on the game's own scan: `scratchpad/radar/saltscan.cpp`): from home and the three C-series
  worlds every step adds one to three peoples again (home 3, +2, +3, +2, +1, +1), peoples of the four radio ages heard from home, its
  farthest 5,524 ly at x32 as before.
* **The test worlds**: the modern age world was Rieneseirela VI-a's second people (medieval now with its first): `peoples ages` pins
  Yeesheleemiola II-b (208 51 body 5, 13.9 ly, two modern peoples; the modern scene's world and the drone bay's). H-02's worlds of two
  move to the nearest that met (Leanmya V-a, 179 57 body 18) and the nearest with a conquest (Hiameshseine I-a, 202 9 body 5): Eleinewai
  I's and Meareafionris III's peoples of one age never meet now. C-13's check reads each people naming the other where they met
  (Eleinewai I's never did, Leanmya V-a's did). T-03's dam holding its water was on Rieneseirela VI-a: the works' checks read Shioleesein
  I-a too (196 66 body 7, the nearest living world whose dams hold water). A new check: the drones bring of both peoples of a world of two
  in one trip (185 of 200 hauls).
* **Saves**: no generation change (13 holds: 1.3.0 is one generation and unreleased). The user's guide touches eleven civilised worlds,
  two of them of two peoples (BUBAS, XOHORT): every one as it was (their peoples' ages, ends and decay printed before and after).

## Verified

* **The user's worlds** (a scratch program over a copy of their guide: every world its shards, instruments, graves, names, marks and
  signals touch): the eleven civilised ones print the same peoples, ages, ends and decay before and after (BUBAS industrial 609 and 230
  years ago, XOHORT medieval 1,651 for both).
* **The worlds of two** (600 near home on the plane): every one of one age now; 466 had a second people of another age.
* **The records**: the grammar's every check at zero over the test worlds' 1,226 records (`grammar check`: the anachronism gone).
* **The drone bay and the case** (`drones check`): Yeesheleemiola II-b's 295 items of two peoples, the hauls, both peoples brought (185 of
  200 hauls of both), the case three folders, the world of two's two opened in turn, each its people's alone (five and six)
  (`shots/tests/drones_case_worlds.png`, `drones_case_world_second.png`).
* **The radar** (C-15's check): every step one to three from home and the C-series worlds (home 3 +2 +3 +2 +1 +1), x1 C-07's with the
  pinned three, 22 at x32 from home, 14 far, the farthest 5,524 ly; C-16's: peoples of the four radio ages heard from home.
* `unit` 0 failures (313: two new), `regress` unchanged (22 hashes), `flow` 37.5 s, `bench check` every budget (the people's descent 72.4
  ms of 80, the capital 5.6 of 16), `peoples ages`: the pinned test worlds hold.
