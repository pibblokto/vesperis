# B-418: the scanner hears no works

status: resolved 2026-10-11
severity: S2
area: the civilisations (the works: where a people's everyday works are)

## Report

The user, after B-417's monoliths: "There are also no works or it's that they are SUPER rear, I don't know why. Whenever I am on the
planet scanner for works says "nothing in 40 km" no matter where I go, always. Haven't seen them since they were added once, should be
also handled. Two small questions - do they also follow the genome and are therey still these megacities/capitals you shared screenshots
of?"

## Cause

T-03 made every world's works once (`buildWorldWorks`), each rule a handful a people: the quarries in the rock nearest each stone-built
town (24 at most), the necropolises by the capital and the cities (four), the mines the history sank and on the territory's three richest
cells, four wrecks by the harbour towns, three henges by a prehistoric people's seats, one to three wind farms, three observatories, three
lighthouses, three aqueducts, five dams, eight battlefields. T-01's plan gives a people hundreds of places over a third of its world or more
(BUBAS's first people: a capital, 74 cities and 302 towns over 129 million km2), and B-414 took the earlier ages' works away. Over the first
41 people's worlds near home: 15 to 126 works a world; 1,083 of the plans' 11,095 places (10%) and 105 of 19,200 points of the peoples' land
(0.5%) had a work within the scanner's 40 km. On BUBAS 45 of the first people's 377 places, none of 400 points of its land. The unit's own
check walked the explorer to the nearest work when none was within reach of its landing.

## Fix

**A people's everyday works are its land's** (`landWorkOf`, galaxy/works.h). The world's list keeps the history's works and the world's
great ones (the battlefields, the dams, the aqueducts, the observatories, the lighthouses the history lit, the launch sites, the holy place
and its way, the colossus, the fortified line, the antenna field, the terraformed basins, the far future's, the lines on the plains, the
last shelter, the mines the history sank); the everyday kinds are the land's, on a grid of 8 km cells:

* **A cell holds one** where its hash falls under the countryside's density at its middle (`planCountryside`: the people's land, thicker
  near its places, nothing beyond its frontier) times its age's appetite (`LAND_RATE`).
* **Its kind is what the cell's ground offers that age** (a 5 x 5 reading of it and the plan's coarse grid): a prehistoric people's flint
  pits, standing stones and a hill fort; a medieval to modern people's quarry (a face or a pit where a stone-built people's rock shows, a
  pit on any dry ground: clay, marl, gravel, sand), a mine where the plan read coal and ore, a ship wrecked on a shore (a modern people's
  aircraft on open ground), a lighthouse on a headland by a settlement, a modern people's wind farm on a ridge, a fort; a futuristic
  people's wreckage fallen from orbit; a necropolis facing a village or more where the people's genome buries its dead, a shrine where the
  ground is its holy place's kind. One is drawn by their weights; one that finds no site gives way to the next.
* **It is sited by its kind's rule inside the cell** (its reach and 60 m inside: no two touch), clear of the settlements, the cities'
  cores and the world's works, made the first time it is asked for (a few milliseconds) and kept (the last two worlds).
* **The fort** (`WO_FORT`, `A FORT`) is new, so that a people whose land offers little (flat forest, no rock, no coal, no burials) left
  something: a prehistoric people's hill fort (rings of bank in their ditches round its last huts), a medieval people's castle (a curtain
  wall of five to eight sides, its corner towers, the gatehouse toward the settlement it guarded, the keep; a moat on the flat), an
  industrial people's redoubt (a ring bank round a casemate), a modern people's strongpoint (pillboxes round a blockhouse, dragon's teeth);
  its towers and walls are the people's genome's, and it keeps a record in its keep or casemate.
* **Who reads them**: `worksAround` gives the world's and the land's within a reach (the scanner's `worksHeardNear`, the landing's
  `settlementClearance`), `worksAroundIfReady` the view's (`WorkNear` holds a shared `Work`; the view never waits for the world's).

## Verified

* **The scanner from the peoples' land** (the unit's new check): from every one of 216 points of the age worlds' prehistoric to modern
  peoples' land and 108 of their places, a work within 40 km (11.2 heard on the whole). Over the sixteen worlds of the survey: 39 or 40 of
  40 points of every medieval to modern people's land, 31 to 40 of the prehistoric; a futuristic people's land only round its great
  places (KI-396). BUBAS: 60 of 60 points of each of its peoples' land (10.2 and 10.5 heard), all forty of its first places; from the
  user's CITY mark 8 works within 40 km (a pit in benches 4.9 km off, barrows, a redoubt), from the MUSICAL CITY 9 (quarries, barrows,
  headframes, a redoubt). A first scan of a region 5 to 10 ms on the scanner's thread (20 at the most).
* **The land's sound** (the unit's new check): 502 of the land's works round the test worlds' first six places and at ten points of their
  land, none over its cell's edge, none touching another, none on a settlement, a core or a world's work, every third the same built again;
  169 quarries each a face in the rock of a stone-built people or a pit under the ground; 22 of 22 kinds laid; every work heard from its
  middle.
* **The works gallery** (`works gallery`): each age's fort (a ring bank through a forest, a castle with its gatehouse and keep, a redoubt, a
  strongpoint), an industrial pit in benches, a modern open pit, barrows, tombs, towers of silence, an aircraft, a wind farm.
* **The view**: it makes the land's works within 8 km of the explorer (`SurfaceView::LAND_NEAR_M`: it draws a work within four kilometres
  of its edge and puts its ground in within six), about 2 ms of a landing's first frame (the people's descent 61.9 ms without the land's
  works, 64.3 with them, run in turns); at 24 km it had cost 6 to 9 ms.
* `unit` 0 failures (297 checks), `flow` exit 0 (35.2 s), the regress unchanged (22 hashes), `bench check` within every budget (the people's
  descent 63.7 ms of 80, the capital 8.0 of 16).
