# B-419: a people's towns moved with the season

status: resolved 2026-10-11
severity: S1
area: the civilisations (what is made on a world: the plan, the settlements, the works; generation against the season)

## Report

Found while fixing B-418: the drone over a mine the scanner heard on BUBAS (450 m off, from `worksHeardNear`) showed nothing, and the
view's list of works there held a different mine 7 km away. Not reported by the user; it would have been, as soon as they came back to a
marked town in another season and found nothing at their mark.

## Cause

The season is `BodyGen::season`: the winter hemisphere's ice cap reaches 11 degrees further and its snow line drops 900 m (M9-09), the
sea under the cap freezes. The generators the game hands the readers of the world carry the season of the moment: the approach's
(`SpaceRenderer::genFor`, made in the season of its step and kept) and the site's (`SurfaceSite::init(..., t)`). T-01's plan reads the planet
(`readPlanGrid`: the land, the biomes, the materials) with whichever generator asks first in a session, and keeps it by the world's seed;
so did the people's extent (`landShareOf`: the land the people held, its numbers, its records' count), the histories, the divide between
two peoples, the settlements' sites (`ruinSiteOk`, `readSite`), the roads' walks, T-03's works and B-418's land's works, and the scanner's
sights. A winter generator saw frozen sea as land and snow as ice sheet: on BUBAS 1,383 of the 1,400 places of its two peoples' plans moved
between a winter's and a summer's generator against the year's mean, its first people 377 places at the mean, 364 in winter, 369 in summer
(Aieliaalas II: 414, 366, 371; Rieneseirela VI-a's second people 455, 355, 416). A session's first approach decided where a people's towns
stood until the plan left the cache; the next session in another season moved them, and the explorer's marks with nothing under them. The
user's BUBAS marks matched the year's mean (B-415's check found their towns there): their approach fell near an equinox.

## Fix

What is made on a world is the world's, not its season's (`unseasoned`, galaxy/planetmap.h: the generator itself where its season is 0,
else a copy at 0). Every reader of the world's made things takes it at its entry: the plan (`readPlanGrid`, `planScores`, `planPeople`,
`buildWorldPlan`, `worldPlanOf`, `worldPlanPrefetch`, `planEdgeWay`, `planSpansIn`), the people's extent (`landShareOf`) and divide
(`peopleDivide`), the histories (`buildHistory`, `historyOf`, `historyOfWorld`), the settlements (`ruinOfCell`, `settlementOf`, `readSite`,
`ruinSiteOk`, `settlementClearance`), the roads (`roadNodeOfCell`, `roadJoined`, `roadWay`, `roadsOfCell`, `roadsNear`, `roadLegWalk`), the
works (`buildWorldWorks`, `workSpec`, the land's context), the scanner's sights (`sightsOfCell`, `ruinsNear`). The terrain the explorer sees
keeps its season (the snow, the sea ice, the planet's maps from orbit).

## Verified

* The plans of five worlds from generators made at the winter solstice, the summer solstice and the year's mean: every place the same (0
  of 4,474 moved), every grid cell's land, biome and material the same.
* The unit's new check (in `works check`): Meihimeeri III's plan, its first people's extent, the settlements and the land's works of the 25
  cells round its capital, and its world's works the same from a winter's and a summer's generator as from the mean.
* BUBAS: the mine the scanner hears 450 m off the drone's spot is the one the view draws (the view's land's works now made as the
  scanner's are), its headframes, engine houses and spoil.
* `unit` 0 failures (297 checks), `flow` exit 0 (35.2 s), the regress unchanged (22 hashes), `bench check` within every budget (the
  people's descent 63.7 ms of 80, the capital 8.0 of 16).
