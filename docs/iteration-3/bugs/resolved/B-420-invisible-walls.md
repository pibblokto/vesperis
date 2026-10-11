# B-420: invisible walls

status: resolved 2026-10-11
severity: S2
area: the surface (the colliders of the settlements' and the works' pieces, the rocks; walking on what is drawn)

## Report

The user, asking for R-412: "Thoroughly test for instruments/shards placements and for invisible walls since I encountred a few". No
place was named; the test is the harness's.

## Cause

The explorer collides with discs (`formColliders`: each a circle with the heights it spans) and sees meshes (`formMesh`); the two were
written apart, and where they disagreed the explorer met a wall where nothing was drawn. A new measure (`walls forms`: every piece of
every place and work of the age worlds, the scene worlds, BUBAS and the peoples nearest home, against its own mesh on flat ground; a disc
a wall where its rim lies outside the drawn solid at the feet) found 5.5% of the discs at the feet over nothing drawn:

* **A turned form's discs were its widest outline.** A lathe's or a pyramid's slices took the foot's radius up the whole height (a cone,
  a tapering chimney, a spire stood a disc of their foot's width at the head's height); a lying one's ignored the rest lift and the break;
  a blob's its lobes.
* **A leaning, lying or broken box** kept its upright footprint (a fallen stone across the ground, a broken flat slab's steps, a jamb
  leaning out).
* **A vault's flanks, an arch's piers, a truss's legs, a torus's tube, a sweep's faces** were each discs at their foot's place up their
  whole height: a parabolic vault's flank a metre inside its shell at the head, a snapped pier's top over nothing, a bent mast's legs
  where they had stood, a standing ring's tube across its hole, a battered dam's foot up its crest.
* **A big piece's foot**: the view stands a piece over 6 m on the highest ground under its footprint with a skirt down to the lowest, its
  colliders on the ground at its middle (a barn on a slope had its walls' discs under the drawn walls or a metre outside them).
* **Earthworks** (a barrow, a bank, a spoil heap, an amphitheatre's tiers) were rings of discs round their foot: a wall where the slope
  only began.
* **Rocks**: the boulders drawn and the boulders collided with were two enumerations (`drawObjects` kept some by their size on screen,
  `collectColliders` others by their size), and boulders stood in settlements, on roads and on worked ground.
* **Meshes with holes**: a box without its underside (a tilted block's inside seen), lathes' holes in the walking band, lying lathes open
  at their ends, pyramids' and sweeps' skirts leaning with their faces.

## Fix

* Every kind's colliders follow its mesh (`galaxy/forms.cpp`): a lathe, a blob or a pyramid upright in slices (its foot, the knee, the
  head, then every 8 m to 16 m) each by its narrowest outline there inset by the disc's radius (`pyramidOutline` a pyramid's corners and
  wear), leaning by its lean, lying by its section where it rests (the break and its jags cut away); a box by its steps, tipped by its
  tipped section, leaning in 2 m pieces; a vault's flanks at the shell's middle at the knee and the head; an arch's piers as wide as the
  piers and as high as a snapped one stands; a dish's mast only where the dish is up; a truss's legs leg by leg below the buckle, through
  its tilt; a torus's tube where it stands; a sweep's faces band by band, its ends inset. `formWalkable` forms (heaps, banks, barrows,
  tiers) carry no discs: the explorer stands on their drawn surface (`formSupportAt` through the view's `floorAt`: a step of 0.5 m, a
  slope over 1.35 barred).
* One foot for drawing and colliding (`SurfaceView::pieceFootNear`), one enumeration of the rocks (`forRocks`: none in settlements,
  on roads, by marks, on edits or in water).
* The meshes closed where they were seen into (`boxMesh`'s underside, lathes' holes over the walking band, lying lathes closed, upright
  skirts under pyramids and sweeps, sweeps' true end caps).
* R-412's (found on the way): a truss's foot goes down its leg's own line for the skirt (pushed straight down, a squat stump's leaning
  legs bent away from their discs) and a leaning leg's discs come in short steps, each as high as the leg stays inside it; a lying piece
  cannot float (a nacelle drawn a hundred metres from its discs).

## Verified

* `walls forms`: 7.5 million discs over 13,173 places and works, 115 at the feet over nothing drawn (0.002%: KI-398's rare forms),
  0.3% above (the jetpack's: holes in snapped towers); it was 5.5% at the feet.
* `walls view` and `walls view near 40`: the walks from round each place stopped by nothing undrawn (see B-421 for the numbers).
* The unit's checks (`walls check`): the colliders over what is drawn (at most one in 20,000 at the feet), 154 walks through the scene
  worlds' places stopped only by what is drawn, 190 boulders that block none on a bed, a barrow walked up and over.
* KI-398: a few rare forms' approximations (a lying wreck, a standing ring, a saddle shell's corners) and the holes of snapped prism
  towers for the jetpack.
