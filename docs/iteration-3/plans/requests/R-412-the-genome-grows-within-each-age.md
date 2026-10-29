# R-412: the genome grows within each age

status: done 2026-10-11 (released in 1.3.0)
milestone: 1.3.0 (after C-17; with B-420 and B-421, the same request's checks)

## Request

"Next thing - now when we have made architecture more representative of the stage and repetiotion slightly increase I want you to
significantly increase genome traits while keeping it easy to recognize what state it is. You can even add additional forms if you
like along with traits. Also works should follow it as well. Thoroughly test for instruments/shards placements and for invisible walls
since I encountred a few"

B-415 made the age speak first (an industrial people's brick, slate and sash windows; a modern people's flat roofs, wide glass and
concrete): every age reads as itself, but within an age the peoples' everyday buildings drew from fewer of A-04's thirty-five variables
(B-415 took the profile, the tops, the finish and much of the colour away from the everyday buildings of the machine ages), and towns of
one age looked more alike. The checks the request asks for are B-420 (the invisible walls) and B-421 (the records and the instruments).

## What was done

Sixteen new deep variables (`galaxy/genome.*`, fifty-one in all; each a hash of its own, so none of A-04's moved), every one read through
the age as B-415 reads the rest: its expression is the age's, its choice the people's. They act on a dwelling's parts the eye picks out at
the scale of a street (a roof's pitch and colour, what rises from it, what stands out from a front, the doorway, the corners, the yard)
rather than on the mass the age sets.

| Group | Variable | Values | What it does | Ages |
| --- | --- | --- | --- | --- |
| Roofs | PITCH | shallow .. steep | every pitched, hipped and conical roof's rise and its gables' (0.62 .. 1.38 of the age's), a hut's cone | prehistoric to industrial |
| | EAVES | flush, overhanging, deep, flared | the eaves' overhang (0.12 to 1 m), a bellcast skirt under them (flared); a hut's thatch; a modern people's slab flush, a lip, deep (1.25 m) or a cornice | prehistoric to modern |
| | ROOFLINE | plain, dormers, gablets, crested, lanterns | dormers along the front slope, a cross gable in its middle, cresting along the ridge (a medieval people's pales of its roofing, an industrial people's iron), a bell-cote or a ventilator; on a hand-made people's flat roof a pavilion, a stair house, a balustrade, a cupola; on a modern people's a pergola, a water tank on its legs, rails round the edge, a penthouse | medieval to modern |
| | GABLES | plain, stepped, curved, pedimented, half-hipped | the gable's end: crow steps over the slope, a curved neck over the ridge under its cap, a pediment's cornice and raking frame, the top clipped by a little hip | medieval, industrial |
| | ROOFING | straw, terracotta, ochre, brown, slate, charcoal, moss, verdigris | the roofing's colour in the age's material (thatch, reed and turf; tiles, shingles, slate, copper; the industrial age's slate, tiles and copper): palette slot 2 | prehistoric to industrial |
| Walls | POLYCHROMY | uniform, two-tone, varied, striped | a darker dado under the first storey's windows (a plinth, a rusticated ground storey, a base), each house its own shade and a third of them in the accent (`Gen::tone`, `Gen::painted`), courses of two shades (striped stone, polychrome brick, bands); a lighthouse's painted bands | medieval to futuristic |
| | TRIM | none, light, dark, shutters | the windows' surrounds lighter or darker than the wall, or shutters beside them (`WindowSpec::frame`, `shutter`) | medieval to modern |
| | PROJECTIONS | none, balconies, galleries, bays, oriels | balconies on the upper storeys (timber, iron, concrete), a gallery along each (a verandah on posts under the first; a one-storey house's verandah; a modern block's access decks), bay windows from the ground, oriels on their corbels; a pod's ring gallery, glass blisters, oriels | medieval to futuristic |
| | ENTRANCES | plain, porch, steps, portico, canopy | the doorway's porch on two posts under a little gable, a stoop of three steps (iron rails, concrete cheeks), a portico of columns under its pediment (a modern people's slab on columns), a hood or an iron or concrete canopy; a hut's porch passage, threshold, portal posts or hood; a pod's arch, step, pylons or shell | all but the far future |
| | CORNERS | plain, quoins, pilasters, turrets | quoins up the front's corners (lighter), pilasters with their capitals, corbelled turrets under their caps (a low house's round corner tower); a modern people's glazed corners, fins, rounded corners | medieval to modern |
| Town | STOREYS | low .. tall | the dwellings' heights within the age's (0.82 .. 1.38: `Layout::heightK`) | medieval to futuristic |
| | FRONTAGE | eaves to the street, gables to the street | a house's or a villa's footprint turned to the street (wide and shallow, or narrow and deep), an industrial row's roof a gable a house | medieval, industrial |
| | SETBACK | at the street .. in gardens | how far a town's houses stand back from their street (`Building::yard`) | medieval to modern |
| | BOUNDARIES | open, walls, fences, hedges, railings | the yard's front to the street with its gate, or a garden behind a house that stands alone: a low wall, pales, a hedge (a wall on a desert), railings (iron on a low wall, a stone balustrade, steel on a kerb); a ring of dry stone, wattle, thorn or stakes round a hut. Each under 0.9 m, stepped over as C-01's low walls are | prehistoric to modern |
| | FURNITURE | none, wells, lamps, fountains, statues | the street furniture (`BK_FURNITURE`) in each square and beside its long main streets, lamps along them: a well under its roof, an iron pump, a drinking fountain, a column of water; a lantern on its post, an iron lamp post, a street light on its arm, a pylon of light; a fountain's basin and bowls; a figure on its plinth, an abstract one, a ring | medieval to futuristic |
| | POLES | none, totems, banners, skulls, cairns | a prehistoric people's pole by each hut's door: a carved post, a cloth of its accent on a pole, a skull with its horns, a cairn | prehistoric |

* **A new form** (`FK_FENCE`, galaxy/forms.*): a fence along its heading, its pales (pointed), its bars between two rails (spear-tipped),
  wattle hurdles between their stakes, or a balustrade's balusters between its plinth and its handrail; each pale where its spacing spans
  a pixel and a half, else each stretch one band; broken, pales and stretches of them gone; its colliders discs where its pales stand. The
  yards' fences and railings, the balconies' and galleries' rails, the ridges' cresting, the flat roofs' rails and balustrades.
* **A new building kind** (`BK_FURNITURE`, its style the `Furniture`), laid last in a town (`placeFurniture`, a stream of its own: nothing
  of the plan moves for it) and round a futuristic people's great structure; no room, no records.
* **The works follow it**: every work's pitched roof takes the pitch, the eaves, the gables and the roofing; a lighthouse its section,
  taper, painted bands, lantern cap and gallery rail, its keeper's house the people's doorway, corners and roofline; a headframe of
  iron lattice, a brick tower over the shaft or an A-frame by the structure; a wind turbine on a lattice tower where the people expressed
  its frames, its nacelle rounded where its roofs were domes or vaults, two to four blades by its number; an observatory's drum and dome
  the people's; a pillbox and a casemate in the people's section; a tomb's gables and doorway; a hangar's roof the people's; a church's,
  a town hall's, a station's and a mill's doorways, corners and roofline.
* **Found on the way**: a town's house turned its door to its street after its pieces were tested against the streets (a porch stood in
  a lane: `fronting` sets its door and yard first); a lying form cannot float (a nacelle drawn a hundred metres off its collider); a truss's
  foot pushed straight down for its skirt bent a squat stump's legs away from their colliders (it goes down the leg's own line now, and a
  leaning leg's discs come in short steps); a polygon pod's dress laid on a circle floated off its flat sides; a balcony on a sunk house
  stood at the ground before its doorway; a hill fort's ring with no gate and its banks lifted to cliffs downhill (B-421).

## Verified

* `genome reach`: every one of the fifty-one variables reaches the drawing in every age it says it acts in (the unit's check).
* `genome dress [scale] [age|all] [desert|felisian] [high] [years]` (new): each of an age's own variables a row, its values across, on one
  plain genome (`shots/tests/genome_dress_<age>_<scale>x.png`): every value read in the five ages it acts in.
* `genome floats` (new): the A-03 check's pieces on nothing and doorways shut tallied by what they are: none.
* The distinctness: the hundred peoples nearest home 10.0 of 19.8 apart at the nearest (alike under 4.9); none of 200,000 random pairs
  under 4.5.
* B-420's and B-421's checks over the new pieces: `walls forms` 115 of 7.5 million discs at the feet over nothing drawn; `walls view
  near 40` 2,474 walks stopped by nothing undrawn, 1,592 records and 106 instruments all reached; the unit's four checks.
* `unit` 0 failures (301 checks), `flow` 37.0 s, the regress unchanged (22 hashes: its scenes hold no settlement), `bench check` within
  every budget (the people's descent 71.0 ms of 80, the capital 12.0 of 16, the same with the new pieces switched off: the capital's view
  from the bench's spot holds few of them).
