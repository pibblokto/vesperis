# B-426: a capital's records heaped on its archive's floor

status: resolved 2026-10-18 (1.3.2, the third of the patch's items)
severity: S3
area: where the records lie (`galaxy/records.cpp`, `recordSitesOf`)

## Report

The user, with two screenshots from the capital of Yaiphuyiabrus IV (PARSIS -31900 +3 -35668, the visit of B-425: the HUD at 3.9°N
99.4°W, EPOC 6011:003.601), rows of carved stones and carved bones on the grass between low walls, `CARVED STONE OF THE OLD PEOPLE - 19
OF 80 ON THIS WORLD`: "Next thing I was noticing A LOT especially in capitals - shards were all located in the center, tons of them [...]
I don't know, maybe it was intentional and I like the idea of the center having a bit more shards sicne you naturally gravitate towards
the center and it's a good reward but the amount of them seems like there is some bug with placement that ignores other buildings and just
stockpiles everything in one place".

## Cause

* **The archive took every record its place kept.** T-04 gave each record a home (`recordHomesOf`: a place of the plan standing in its
  year, the capital weighing ten, a city four, a town one and a half, a village one) "which always holds it (its archive, else its rooms)",
  and the archive its class's count (the capital's 8 to 14, a prehistoric capital's 4 to 6, a city's 5 to 8, a third of the towns' 3 to 5:
  `docs/reference/civilisations/built-world.md`). `recordSitesOf` asked the archive for the larger of its count and the place's own (`std::max(k,
  own)`), so it laid them all in its rows (four of 24 at most, half a metre apart across the room). A people of few places left its capital
  a third to a half of its records: a prehistoric people's capital among its villages, a futuristic people's among a handful of towns.
  Yaiphuyiabrus IV (a prehistoric people, 80 records, a capital and forty villages): the capital kept 38; its archive took 31 in two rows
  across its chief's longhouse (15 by 51 m at the centre, its timber long gone, so the rows lay on the grass between its low walls) and nine
  huts had one each. Over the 200 peoples nearest home the prehistoric capitals' archives held a median 19 (33 at most), the futuristic
  capitals' 32 (40), a medieval capital's up to 20; the industrial and modern capitals and every city kept their counts (a large plan
  leaves each place few of its own).
* **A lone stone's foot held two.** A place's own records beyond the spots of its buildings' rooms were left out: a prehistoric village of
  a lone menhir (two spots at its foot) or of a stone circle and an aqueduct. Over the same survey 29 records of the 228 prehistoric
  villages and 3 of the futuristic towns lay nowhere on the ground (the drones and the landing map's homes still had them at their place).

## Fix

* **An archive holds its count**, the place's own first, as T-04 wrote it; the place's own beyond it lie in its other buildings, one a
  building by C-03's weights (the place draws as many buildings as it keeps records), then a second and a third in a room's spots.
* **Every place holds its own.** What the rooms' spots cannot hold goes one at a time across the room (or round the foot) of each building
  holding one, after what lies there, and the archive's rows take the last (`archive(..., rest)`: from the place's own alone, a painted
  record as a slab where a slab has room). The far future's structures, a building alone, keep theirs in rings round their feet as before
  (the archive that keeps itself).
* No generation change: a found record keeps its index (the guide's key) wherever it lies; only the places' layouts of their records move.

## Verification (2026-10-18)

* `vesperis_test unit`, a new check (B-426): over the six age worlds' places and Yaiphuyiabrus IV's, 443 archives and none over its
  class's count (the most 14, a futuristic capital's), 902 of the places' 902 own records laid at them, the far future's rings 175; the
  reported capital an archive of 4 (31 before), its 38 own laid in 34 buildings, 4 at most within 6 m of one another (30 before). The check
  fails on the old placement (3 archives over, the most 31; 900 of 902; 30 within 6 m).
* The survey of the 200 peoples nearest home before and after (a scratch tool beside the harness; 7,164 places): the archives' median and
  most by class, prehistoric capitals 19 and 33 to 5 and 6, futuristic capitals 32 and 40 to 13 and 14, medieval capitals 12 and 20 to 11
  and 14, the rest as they were; the most records within 6 m of one in a capital 40 to 15 (a far future structure's ring 21 to 22); the
  places' own records left out 36 to 4. Those four, and one past the survey's sample, are three harbour places' that lay nothing at all:
  every spot of theirs reads as the sea (B-421's rule, the planet function at 16 m), and with the water ignored they would lay 7, 10 and 4
  (KI-415).
* The reported place, before and after, from inside the longhouse and from 30 m over it: a row of records across its whole floor and a
  second behind it; four in its middle.
* `unit` 337 checks, 0 failures; `regress` 24 of 24 unchanged; `flow` passes; `records scenes` (the explorer walks to each age's archive and each common
  medium in its capital and takes them) 0 failures. The other unit lines that moved: the test worlds' 1,328 places hold 6,219 records
  (6,236 before: a capital whose own records fill C-03's few lays no copies of other places' now; the reported one 38, all its own, 41
  before), B-421's scene worlds 121 records and 7 instruments (128 and 8), C-11's instruments 496 beside their records (499).
* A capital's records are laid in 0.6 to 1.8 ms now (0.4 to 1.1 before: more rooms are read for their floors), once a layout.
