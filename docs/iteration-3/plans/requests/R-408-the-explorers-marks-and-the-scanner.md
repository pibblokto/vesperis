# R-408: the explorer's own marks, and a scanner that hints the way

status: done 2026-10-07
milestone: 1.2.0 (after X-04, the user's request on playing it)

## Request

"Another huge thing - landmarks are a bit broken now. When you land you have an option to name nearest landmark once and then
it never gets updated or anything - you may discover another ruins later and you just can't do anything with them unless you
go back to stardrifter and redeploy. What I would like you to do is to remove them as markers, maybe, and make the surface N
map more interactive so you can place arbitrary marker on something YOU see as a landmark and name it - no nearest type of
[thing] autosuggested when you press G and as I said - make N map more interactive. It also brings us to another feature I
didn't mention initially - you see, I was using this feature with nearest landmark to find ruins but it sucked gameplay wise -
land, check with G, redeploy capsule, repeat. What I want you to do is to add some sort of echo locator or scanner or anything
that would hint you in what direction to go to find something potentially interesting - maybe in different modes so you can
tune it for ruins or for water to find lakes or for peaks, etc - this way landmarks persist, we are not dependant on how they
are located automatically - we just place it on the map. And maybe make this surface radar/scanner just ignore selected sights
so we don't get retriggered by them but make it optional - let's say you already found a ruins of the city, named it and all
you want now is to find next nearest ruins - please figure that all out."

O6-06's landmarks were one per 14 km cell (the first of an order of kinds), searched at every landing within the drawn disc
plus 8 km, logged when the explorer came within a kilometre and drawn on the maps from then (B-401); the guide's `NAME THE
NEAREST LANDMARK` named whichever was nearest within 12 km. A cell holding a crater hid its ruins; the ruins nearer than the
cell's biggest were never landmarks at all.

## Done

* **The sights by kind** (`galaxy/landmarks.*`): every cell's candidate of each kind is read on its own (`sightsOfCell`: the
  peak, the crater, the canyon's rim, the mesa, the geyser field, the crystal field, the lake, the biggest ruin, with the
  cell's scan kept for O6-06's order; a lake is now the biggest lake of the cell that holds water, at a point of its water,
  where O6-06 took the centroid of every lake cell, playas and the dry ground between two lakes included), the ruins of the 2 km grid are listed one by one (`ruinsNear`: the settlements as the
  surface view keeps them, the monoliths out of the water), and `sightKindsOf` says which kinds a world can hold. The
  generated names are gone (nothing showed them since R-401). They are no longer markers: nothing is logged or drawn by
  itself.
* **The scanner.** `R` on the ground (in a vehicle too) cycles it through the kinds the world can hold: RUINS, LAKES, PEAKS,
  CRATERS, CANYONS, MESAS, GEYSERS, CRYSTALS, off. It hears every sight of those kinds within 40 km. The search runs on a
  thread of its own: about 50 ms when the drainage tiles are warm, 0.9 s on average and 2 s at most when they are cold. It
  searches again every 5 km the explorer goes. Its lines at the top left: `SCAN RUINS` / `1.6 KM NW` (or `SCANNING...`,
  `NONE IN 40 KM`, `NONE ON THIS WORLD`, `HERE`). The compass strip carries a diamond at the echo's bearing, or an arrow
  at the strip's end toward it. An echo is heard off its place by up to 8% of its distance (3 km at most, a direction of
  its own), so far away it gives a way and near it gives the place. `Shift+R` leaves out or takes in the sights the
  explorer has marked: a sight counts as marked when one of the explorer's own marks stands within its extent (150 m at
  least) and 250 m more. It is left out by default, and `*` after the mode says the marked ones are heard too.
* **Marks** (`Guide::marks`, `mark` lines): a place of a world with a name (or none) and what it marks (a sight's kind and
  glyph, or a place's small diamond). `L` marks where the crosshair rests (the rangefinder's spot, else the feet). On the sector
  map, Enter marks the place under the cursor. Either opens the name's entry: Tab changes what it marks (the default is the
  sight the scanner hears there), Enter places it (an empty name too), Esc places nothing. A mark is logged (`MARK`). The
  marks show on the sector map, the landing map (a dot each) and its zoom (glyph and name). Over the view, the eight nearest
  within 25 km show a diamond with the name and the distance. The rangefinder's second line names the mark the crosshair
  rests on (O6-06's `UNNAMED PEAK` went with the landmarks). A friend's marks travel with the guide's export, are lent in cyan
  like the shards and the recordings, and are not lent on.
* **The sector map** (N) is the place for marks. A cursor moves with the arrows (Shift faster) and the mouse, and C or Space
  bring it back. Enter marks the place under it (on a mark: renames it), Delete twice removes the mark under it, M puts the
  waypoint there (on the waypoint: clears it), Tab goes from mark to mark in the frame, and +/- or the wheel zoom (the arrows
  did). The scanner's echoes of its mode show as dots where they are heard (the nearest 30, the nearest boxed). The panel
  names the cursor's distance, way, place and sector, the mark under it, and the scanner's mode, nearest echo and what it is
  (`A TOWN`, `1240 M, 380 OVER`, `2.3 KM ACROSS`). Esc or N closes it.
* **The guide menu** lost `NAME THE NEAREST LANDMARK`. The explorer's names of O6-06 landmarks (`<world>/L<id>`) become marks
  where those landmarks stand, with their names and kinds, when the ship is in their system (`migrateLandmarkNames`: one pass
  over the world's grid finds the cells by their ids, the old order picks the landmark, on a thread: a copy of the player's
  guide turned all 39 of its names in eight systems into marks, 37 at ruins, a lake, a peak; a system's 12 took 7.5 s cold,
  which on the main thread froze the arrival). A friend's such names become lent
  marks. The `landmark` lines of the old guides are no longer read.
* Harness: five unit checks (lakes are water and peaks are tops round a mountain place; a sight named before is a mark where it
  stands; the scanner hears the ruins with the nearest heard within 8%, a mark from the map's cursor leaves it out and Shift+R
  takes it in; the cursor renames, removes and waypoints, and L marks the rangefinder's spot; the marks through the guide and
  the lending); the `landmarks` mode lists the kinds and the scanner's cost; the flow scans and marks.
* Docs: 06 (the landmarks' section), 07 (the surface keys, the sector map, the guide menu), 08, 10-decisions, 12-civilisations.
